from __future__ import annotations

import argparse
import os
import time

import cv2

from camera import CameraConfig, ReconnectingCamera, source_value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture sample frames for vision development.")
    parser.add_argument(
        "--source",
        default=os.environ.get(
            "CAMERA_URL",
            "rtsp://127.0.0.1:8554/camera",
        ),
    )
    parser.add_argument("--output-dir", default="vision/outputs/captures")
    parser.add_argument("--interval", type=float, default=1.0)
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--headless", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    os.makedirs(args.output_dir, exist_ok=True)
    camera = ReconnectingCamera(CameraConfig(source_value(args.source)))
    saved = 0
    next_capture = 0.0
    try:
        while saved < args.count:
            ok, frame = camera.read()
            if not ok or frame is None:
                continue

            now = time.monotonic()
            if now < next_capture:
                if not args.headless:
                    cv2.imshow("Capture frames", frame)
                    if cv2.waitKey(1) & 0xFF == ord("q"):
                        break
                continue

            filename = os.path.join(args.output_dir, f"frame_{saved:05d}.jpg")
            if not cv2.imwrite(filename, frame):
                print(f"保存失败: {filename}")
                return 1
            saved += 1
            next_capture = now + max(args.interval, 0.0)
            print(f"[{saved}/{args.count}] {filename}")

            if not args.headless:
                cv2.imshow("Capture frames", frame)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break
    finally:
        camera.release()
        if not args.headless:
            cv2.destroyAllWindows()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
