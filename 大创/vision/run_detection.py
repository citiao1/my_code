from __future__ import annotations

import argparse
import json
import os
import time
from datetime import datetime, timezone

import cv2

from camera import CameraConfig, ReconnectingCamera, source_value
from detector import create_detector, draw_detections


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the safe, read-only vision pipeline.")
    parser.add_argument(
        "--source",
        default=os.environ.get(
            "CAMERA_URL",
            "rtsp://127.0.0.1:8554/camera",
        ),
        help="RTSP URL, local camera index, or video file.",
    )
    parser.add_argument("--detector", choices=("none", "color"), default="none")
    parser.add_argument("--output-dir", default="vision/outputs")
    parser.add_argument("--save-interval", type=float, default=5.0)
    parser.add_argument("--max-fps", type=float, default=0.0)
    parser.add_argument("--status-interval", type=float, default=5.0)
    parser.add_argument("--headless", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    os.makedirs(args.output_dir, exist_ok=True)
    log_path = os.path.join(args.output_dir, "detections.jsonl")
    camera = ReconnectingCamera(CameraConfig(source_value(args.source)))
    detector = create_detector(args.detector)
    last_saved = 0.0
    last_frame_time = time.monotonic()
    smoothed_fps = 0.0
    frame_count = 0
    last_status = 0.0

    print("视觉程序已启动：只读视频，不连接 STM32，不发送运动命令。")
    print(f"source={args.source}, detector={args.detector}")
    try:
        with open(log_path, "a", encoding="utf-8") as log_file:
            while True:
                ok, frame = camera.read()
                if not ok or frame is None:
                    continue

                now = time.monotonic()
                delta = max(now - last_frame_time, 1e-6)
                instant_fps = 1.0 / delta
                smoothed_fps = instant_fps if smoothed_fps == 0 else smoothed_fps * 0.9 + instant_fps * 0.1
                last_frame_time = now
                frame_count += 1

                detections = detector.detect(frame)
                annotated = draw_detections(frame, detections)
                cv2.putText(
                    annotated,
                    f"FPS {smoothed_fps:.1f} | detections {len(detections)}",
                    (10, 24),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.65,
                    (255, 255, 255),
                    2,
                    cv2.LINE_AA,
                )

                event = {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "frame": frame_count,
                    "fps": round(smoothed_fps, 2),
                    "detections": [
                        {
                            "label": item.label,
                            "confidence": round(item.confidence, 4),
                            "bbox": item.bbox,
                            "center": item.center,
                        }
                        for item in detections
                    ],
                }
                log_file.write(json.dumps(event, ensure_ascii=False) + "\n")
                log_file.flush()

                if now - last_saved >= max(args.save_interval, 0.0):
                    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    cv2.imwrite(os.path.join(args.output_dir, f"raw_{stamp}.jpg"), frame)
                    cv2.imwrite(os.path.join(args.output_dir, f"annotated_{stamp}.jpg"), annotated)
                    last_saved = now

                if now - last_status >= max(args.status_interval, 0.0):
                    print(
                        f"[status] frames={frame_count} fps={smoothed_fps:.1f} "
                        f"detections={len(detections)}",
                        flush=True,
                    )
                    last_status = now

                if args.max_fps > 0:
                    time.sleep(max(0.0, 1.0 / args.max_fps - (time.monotonic() - now)))

                if not args.headless:
                    cv2.imshow("Orange Pi vision", annotated)
                    if cv2.waitKey(1) & 0xFF == ord("q"):
                        break
    except KeyboardInterrupt:
        print("\n收到退出信号。")
    finally:
        camera.release()
        if not args.headless:
            cv2.destroyAllWindows()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
