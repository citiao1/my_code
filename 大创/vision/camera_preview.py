from __future__ import annotations

import argparse
import os

import cv2

from camera import CameraConfig, ReconnectingCamera, source_value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Preview the Orange Pi RTSP camera.")
    parser.add_argument(
        "--source",
        default=os.environ.get(
            "CAMERA_URL",
            "rtsp://127.0.0.1:8554/camera",
        ),
        help="RTSP URL or a local video device/file.",
    )
    parser.add_argument("--headless", action="store_true", help="Save one frame instead of opening a window.")
    parser.add_argument("--output", default="vision/outputs/preview.jpg")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    camera = ReconnectingCamera(CameraConfig(source_value(args.source)))
    try:
        ok, frame = camera.read()
        if not ok or frame is None:
            print(f"无法读取视频源: {args.source}")
            return 1

        print(f"视频源正常: {frame.shape[1]}x{frame.shape[0]}")
        if args.headless:
            os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
            if not cv2.imwrite(args.output, frame):
                print(f"保存失败: {args.output}")
                return 1
            print(f"已保存: {args.output}")
            return 0

        while True:
            ok, frame = camera.read()
            if not ok or frame is None:
                continue
            cv2.imshow("Orange Pi camera preview", frame)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break
    finally:
        camera.release()
        if not args.headless:
            cv2.destroyAllWindows()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
