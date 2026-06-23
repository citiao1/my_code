#!/usr/bin/env python3
"""Make images or animated GIFs symmetric from the center line."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageOps, ImageSequence


IMAGE_EXTENSIONS = {
    ".png",
    ".jpg",
    ".jpeg",
    ".jpe",
    ".jfif",
    ".webp",
    ".bmp",
    ".tif",
    ".tiff",
    ".gif",
}


def mirrored_frame(image: Image.Image, axis: str = "vertical", source: str = "left") -> Image.Image:
    """Return a copy with one half mirrored across the center line."""
    frame = image.convert("RGBA")
    width, height = frame.size
    result = frame.copy()

    if axis == "vertical":
        if source == "left":
            half_width = (width + 1) // 2
            half = frame.crop((0, 0, half_width, height))
            result.paste(ImageOps.mirror(half), (width - half_width, 0))
        elif source == "right":
            half_width = (width + 1) // 2
            half = frame.crop((width - half_width, 0, width, height))
            result.paste(ImageOps.mirror(half), (0, 0))
        else:
            raise ValueError("For vertical symmetry, source must be 'left' or 'right'.")
    elif axis == "horizontal":
        if source == "top":
            half_height = (height + 1) // 2
            half = frame.crop((0, 0, width, half_height))
            result.paste(ImageOps.flip(half), (0, height - half_height))
        elif source == "bottom":
            half_height = (height + 1) // 2
            half = frame.crop((0, height - half_height, width, height))
            result.paste(ImageOps.flip(half), (0, 0))
        else:
            raise ValueError("For horizontal symmetry, source must be 'top' or 'bottom'.")
    else:
        raise ValueError("axis must be 'vertical' or 'horizontal'.")

    return result


def default_output_path(input_path: Path) -> Path:
    return input_path.with_name(f"{input_path.stem}_symmetric{input_path.suffix}")


def iter_input_files(paths: Iterable[Path]) -> list[Path]:
    files: list[Path] = []
    for path in paths:
        if path.is_dir():
            for child in sorted(path.iterdir()):
                if child.is_file() and child.suffix.lower() in IMAGE_EXTENSIONS:
                    files.append(child)
        elif path.is_file():
            files.append(path)
        else:
            raise FileNotFoundError(path)
    return files


def save_animated_image(image: Image.Image, output_path: Path, axis: str, source: str) -> None:
    frames: list[Image.Image] = []
    durations: list[int] = []

    for frame in ImageSequence.Iterator(image):
        frames.append(mirrored_frame(frame, axis=axis, source=source))
        durations.append(int(frame.info.get("duration", image.info.get("duration", 100))))

    if not frames:
        raise ValueError("No frames found in animated image.")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    save_kwargs = {
        "save_all": True,
        "append_images": frames[1:],
        "duration": durations,
        "loop": int(image.info.get("loop", 0)),
    }

    try:
        frames[0].save(output_path, **save_kwargs)
    except ValueError as exc:
        if "palette" not in str(exc).lower():
            raise

        # Some GIFs hit Pillow's palette handling edge cases after RGBA edits.
        # Flattening to RGB loses GIF transparency, but keeps the animation usable.
        rgb_frames = [frame.convert("RGB") for frame in frames]
        rgb_frames[0].save(output_path, append_images=rgb_frames[1:], **save_kwargs)


def process_image(input_path: Path, output_path: Path, axis: str, source: str) -> Path:
    with Image.open(input_path) as image:
        is_animated = bool(getattr(image, "is_animated", False))
        output_path.parent.mkdir(parents=True, exist_ok=True)

        if is_animated or input_path.suffix.lower() == ".gif":
            save_animated_image(image, output_path, axis=axis, source=source)
            return output_path

        result = mirrored_frame(image, axis=axis, source=source)
        save_kwargs = {}
        if output_path.suffix.lower() in {".jpg", ".jpeg", ".jpe", ".jfif"}:
            result = result.convert("RGB")
            save_kwargs["quality"] = 95
        elif output_path.suffix.lower() == ".webp":
            save_kwargs["quality"] = 95

        result.save(output_path, **save_kwargs)
        return output_path


def resolve_outputs(inputs: list[Path], output: Path | None) -> list[Path]:
    if output is None:
        return [default_output_path(path) for path in inputs]

    if len(inputs) == 1 and not output.is_dir():
        return [output]

    output.mkdir(parents=True, exist_ok=True)
    return [output / f"{path.stem}_symmetric{path.suffix}" for path in inputs]


def run_gui() -> int:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    root = tk.Tk()
    root.title("图片/动图中线对称工具")
    root.geometry("520x300")
    root.resizable(False, False)

    selected_files: list[Path] = []
    axis_var = tk.StringVar(value="vertical")
    source_var = tk.StringVar(value="left")
    output_var = tk.StringVar(value="")
    status_var = tk.StringVar(value="请选择 PNG、JPG、WebP、GIF 等图片。")

    def choose_files() -> None:
        names = filedialog.askopenfilenames(
            title="选择图片或动图",
            filetypes=[
                ("Images", "*.png *.jpg *.jpeg *.jpe *.jfif *.webp *.bmp *.tif *.tiff *.gif"),
                ("All files", "*.*"),
            ],
        )
        if names:
            selected_files.clear()
            selected_files.extend(Path(name) for name in names)
            status_var.set(f"已选择 {len(selected_files)} 个文件。")

    def choose_output_dir() -> None:
        name = filedialog.askdirectory(title="选择输出文件夹")
        if name:
            output_var.set(name)

    def sync_source_options(*_: object) -> None:
        if axis_var.get() == "vertical":
            source_box.configure(values=("left", "right"))
            if source_var.get() not in {"left", "right"}:
                source_var.set("left")
        else:
            source_box.configure(values=("top", "bottom"))
            if source_var.get() not in {"top", "bottom"}:
                source_var.set("top")

    def start() -> None:
        if not selected_files:
            messagebox.showwarning("还没有选择文件", "请先选择一张或多张图片。")
            return

        output_dir = Path(output_var.get()) if output_var.get() else None
        try:
            outputs = resolve_outputs(selected_files, output_dir)
            for input_path, output_path in zip(selected_files, outputs):
                process_image(input_path, output_path, axis_var.get(), source_var.get())
            status_var.set(f"完成：已输出 {len(outputs)} 个文件。")
            messagebox.showinfo("处理完成", "\n".join(str(path) for path in outputs[:10]))
        except Exception as exc:  # noqa: BLE001 - show GUI-friendly errors
            messagebox.showerror("处理失败", str(exc))
            status_var.set("处理失败，请检查文件格式或输出路径。")

    root.columnconfigure(0, weight=1)

    pad = {"padx": 18, "pady": 8}
    ttk.Label(root, text="图片/动图中线对称工具", font=("Microsoft YaHei UI", 14, "bold")).grid(
        row=0, column=0, sticky="w", **pad
    )
    ttk.Button(root, text="选择图片/动图", command=choose_files).grid(row=1, column=0, sticky="ew", **pad)

    options = ttk.Frame(root)
    options.grid(row=2, column=0, sticky="ew", padx=18, pady=8)
    options.columnconfigure(1, weight=1)
    options.columnconfigure(3, weight=1)

    ttk.Label(options, text="对称方向").grid(row=0, column=0, sticky="w")
    axis_box = ttk.Combobox(options, textvariable=axis_var, values=("vertical", "horizontal"), state="readonly")
    axis_box.grid(row=0, column=1, sticky="ew", padx=(8, 18))
    ttk.Label(options, text="保留哪一半").grid(row=0, column=2, sticky="w")
    source_box = ttk.Combobox(options, textvariable=source_var, values=("left", "right"), state="readonly")
    source_box.grid(row=0, column=3, sticky="ew", padx=(8, 0))
    axis_var.trace_add("write", sync_source_options)

    ttk.Button(root, text="选择输出文件夹（可选）", command=choose_output_dir).grid(
        row=3, column=0, sticky="ew", **pad
    )
    ttk.Label(root, textvariable=output_var).grid(row=4, column=0, sticky="w", padx=18)
    ttk.Button(root, text="开始处理", command=start).grid(row=5, column=0, sticky="ew", **pad)
    ttk.Label(root, textvariable=status_var).grid(row=6, column=0, sticky="w", padx=18, pady=(10, 0))

    root.mainloop()
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Make static images or animated GIFs symmetric from the center line."
    )
    parser.add_argument("inputs", nargs="*", type=Path, help="Input image files or folders.")
    parser.add_argument("-o", "--output", type=Path, help="Output file for one input, or output folder for many inputs.")
    parser.add_argument(
        "--axis",
        choices=("vertical", "horizontal"),
        default="vertical",
        help="Mirror across the vertical center line or horizontal center line.",
    )
    parser.add_argument(
        "--source",
        choices=("left", "right", "top", "bottom"),
        default="left",
        help="Which half should be kept and mirrored to the other side.",
    )
    parser.add_argument("--gui", action="store_true", help="Open the graphical picker.")
    return parser.parse_args(argv)


def should_open_gui(argv: list[str], args: argparse.Namespace) -> bool:
    executable_name = Path(sys.executable).name.lower()
    is_packaged_exe = getattr(sys, "frozen", False) and executable_name.endswith(".exe")
    return args.gui or not args.inputs or (is_packaged_exe and len(argv) == 0)


def main(argv: list[str] | None = None) -> int:
    actual_argv = sys.argv[1:] if argv is None else argv
    args = parse_args(actual_argv)
    if should_open_gui(actual_argv, args):
        return run_gui()

    inputs = iter_input_files(args.inputs)
    if not inputs:
        print("No image files found.", file=sys.stderr)
        return 1

    if args.axis == "vertical" and args.source not in {"left", "right"}:
        print("--source must be left or right when --axis vertical.", file=sys.stderr)
        return 2
    if args.axis == "horizontal" and args.source not in {"top", "bottom"}:
        print("--source must be top or bottom when --axis horizontal.", file=sys.stderr)
        return 2

    outputs = resolve_outputs(inputs, args.output)
    for input_path, output_path in zip(inputs, outputs):
        process_image(input_path, output_path, axis=args.axis, source=args.source)
        print(f"{input_path} -> {output_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
