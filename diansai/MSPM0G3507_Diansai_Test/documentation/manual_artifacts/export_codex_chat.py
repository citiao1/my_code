from __future__ import annotations

import argparse
import base64
import hashlib
import json
import mimetypes
from collections import OrderedDict
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from zoneinfo import ZoneInfo


THREAD_ID = "019f6a0f-be08-71c3-9098-7d32da9f5997"
DEFAULT_SOURCE = Path.home() / ".codex" / "sessions" / "2026" / "07" / "16" / (
    f"rollout-2026-07-16T16-34-09-{THREAD_ID}.jsonl"
)
ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = ROOT / "documentation" / "智能车项目完整聊天记录.md"
DEFAULT_ASSETS = ROOT / "documentation" / "manual_artifacts" / "chat_export_assets"
LOCAL_TIMEZONE = ZoneInfo("Asia/Shanghai")

IGNORED_USER_PREFIXES = (
    "# AGENTS.md instructions",
    "<environment_context>",
    "<codex_internal_context",
    "Another language model started to solve this problem",
)


@dataclass
class ChatMessage:
    timestamp: datetime
    turn_id: str
    role: str
    phase: str
    text_parts: list[str] = field(default_factory=list)
    images: list[str] = field(default_factory=list)


def parse_timestamp(value: str) -> datetime:
    return datetime.fromisoformat(value.replace("Z", "+00:00")).astimezone(LOCAL_TIMEZONE)


def quote_markdown(text: str) -> str:
    lines = text.rstrip().splitlines()
    if not lines:
        return ">"
    return "\n".join("> " + line if line else ">" for line in lines)


def is_ignored_user_message(text_parts: list[str]) -> bool:
    visible = "\n".join(text_parts).lstrip()
    return not visible or visible.startswith(IGNORED_USER_PREFIXES)


def save_data_image(
    image_url: str,
    assets_dir: Path,
    image_names: dict[str, str],
) -> str | None:
    if not image_url.startswith("data:") or ";base64," not in image_url:
        return None
    header, encoded = image_url.split(",", 1)
    mime_type = header[5:].split(";", 1)[0]
    payload = base64.b64decode(encoded)
    digest = hashlib.sha256(payload).hexdigest()
    if digest in image_names:
        return image_names[digest]

    extension = mimetypes.guess_extension(mime_type) or ".bin"
    if extension == ".jpe":
        extension = ".jpg"
    name = f"image-{len(image_names) + 1:03d}{extension}"
    assets_dir.mkdir(parents=True, exist_ok=True)
    (assets_dir / name).write_bytes(payload)
    image_names[digest] = name
    return name


def load_messages(source: Path, assets_dir: Path) -> tuple[list[ChatMessage], int]:
    messages: list[ChatMessage] = []
    image_names: dict[str, str] = {}

    with source.open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            try:
                item = json.loads(raw_line)
            except json.JSONDecodeError:
                continue
            if item.get("type") != "response_item":
                continue
            payload = item.get("payload") or {}
            if payload.get("type") != "message":
                continue
            role = payload.get("role")
            if role not in {"user", "assistant"}:
                continue

            text_parts: list[str] = []
            image_files: list[str] = []
            for content in payload.get("content") or []:
                content_type = content.get("type")
                if content_type in {"input_text", "output_text"}:
                    text = content.get("text") or ""
                    if text:
                        text_parts.append(text)
                elif content_type == "input_image":
                    name = save_data_image(
                        content.get("image_url") or "", assets_dir, image_names
                    )
                    if name:
                        image_files.append(name)

            if role == "user" and is_ignored_user_message(text_parts):
                continue
            phase = payload.get("phase") or ("user" if role == "user" else "assistant")
            metadata = payload.get("internal_chat_message_metadata_passthrough") or {}
            messages.append(
                ChatMessage(
                    timestamp=parse_timestamp(item["timestamp"]),
                    turn_id=metadata.get("turn_id") or "unknown-turn",
                    role=role,
                    phase=phase,
                    text_parts=text_parts,
                    images=image_files,
                )
            )
    return messages, len(image_names)


def render_message_body(message: ChatMessage, assets_relative: Path) -> str:
    chunks = [part.rstrip() for part in message.text_parts if part.strip()]
    for image_name in message.images:
        image_path = (assets_relative / image_name).as_posix()
        chunks.append(f"![聊天附件]({image_path})")
    return quote_markdown("\n\n".join(chunks))


def build_markdown(
    source: Path,
    output: Path,
    assets_dir: Path,
    messages: list[ChatMessage],
    image_count: int,
) -> str:
    turns: OrderedDict[str, list[ChatMessage]] = OrderedDict()
    for message in messages:
        turns.setdefault(message.turn_id, []).append(message)

    user_count = sum(message.role == "user" for message in messages)
    commentary_count = sum(message.phase == "commentary" for message in messages)
    final_count = sum(message.phase == "final_answer" for message in messages)
    generated_at = datetime.now(LOCAL_TIMEZONE)
    assets_relative = assets_dir.relative_to(output.parent)

    lines = [
        "# 智能车项目 Codex 完整聊天记录",
        "",
        f"- 任务 ID：`{THREAD_ID}`",
        f"- 导出时间：{generated_at:%Y-%m-%d %H:%M:%S}（Asia/Shanghai）",
        f"- 原始记录：`{source}`",
        f"- 用户消息：{user_count} 条",
        f"- 助手过程更新：{commentary_count} 条",
        f"- 助手最终答复：{final_count} 条",
        f"- 去重附件图片：{image_count} 张",
        "",
        "> 本文件只保留用户与助手可见消息。系统提示、开发者指令、内部推理、工具调用和环境注入均已过滤。过程更新默认折叠，可点击展开。",
        "",
        "---",
        "",
    ]

    visible_turn_number = 0
    for turn_messages in turns.values():
        user_messages = [message for message in turn_messages if message.role == "user"]
        assistant_updates = [
            message for message in turn_messages if message.phase == "commentary"
        ]
        assistant_finals = [
            message for message in turn_messages if message.phase == "final_answer"
        ]
        assistant_other = [
            message
            for message in turn_messages
            if message.role == "assistant"
            and message.phase not in {"commentary", "final_answer"}
        ]
        if not user_messages and not assistant_updates and not assistant_finals and not assistant_other:
            continue

        visible_turn_number += 1
        first_time = turn_messages[0].timestamp
        lines.extend(
            [
                f"## 第 {visible_turn_number} 轮 · {first_time:%Y-%m-%d %H:%M:%S}",
                "",
            ]
        )
        if not user_messages:
            lines.extend(["_此轮为任务自动续作或上下文恢复。_", ""])

        for message in user_messages:
            lines.extend(
                [
                    f"### 用户 · {message.timestamp:%H:%M:%S}",
                    "",
                    render_message_body(message, assets_relative),
                    "",
                ]
            )

        if assistant_updates:
            lines.extend(
                [
                    "<details>",
                    f"<summary>助手过程更新（{len(assistant_updates)} 条）</summary>",
                    "",
                ]
            )
            for index, message in enumerate(assistant_updates, start=1):
                lines.extend(
                    [
                        f"**更新 {index} · {message.timestamp:%H:%M:%S}**",
                        "",
                        render_message_body(message, assets_relative),
                        "",
                    ]
                )
            lines.extend(["</details>", ""])

        for message in assistant_finals:
            lines.extend(
                [
                    f"### 助手最终答复 · {message.timestamp:%H:%M:%S}",
                    "",
                    render_message_body(message, assets_relative),
                    "",
                ]
            )

        for message in assistant_other:
            lines.extend(
                [
                    f"### 助手消息 · {message.timestamp:%H:%M:%S}",
                    "",
                    render_message_body(message, assets_relative),
                    "",
                ]
            )
        lines.extend(["---", ""])

    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="将 Codex JSONL 会话导出为可读 Markdown")
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--assets", type=Path, default=DEFAULT_ASSETS)
    args = parser.parse_args()

    if not args.source.is_file():
        raise FileNotFoundError(f"找不到 Codex 会话文件：{args.source}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    messages, image_count = load_messages(args.source, args.assets)
    markdown = build_markdown(
        args.source, args.output, args.assets, messages, image_count
    )
    args.output.write_text(markdown, encoding="utf-8", newline="\n")
    print(args.output)
    print(
        json.dumps(
            {
                "messages": len(messages),
                "users": sum(message.role == "user" for message in messages),
                "commentary": sum(message.phase == "commentary" for message in messages),
                "final_answers": sum(message.phase == "final_answer" for message in messages),
                "images": image_count,
            },
            ensure_ascii=False,
        )
    )


if __name__ == "__main__":
    main()
