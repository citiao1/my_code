from __future__ import annotations

import argparse
import json
import os
import time
from datetime import datetime, timezone

import cv2

try:
    from .camera import CameraConfig, LatestFrameCamera, source_value
    from .detector import Detection
    from .face_detector import YuNetFaceDetector
    from .face_tracker import FaceTargetTracker, OffsetSmoother, calculate_offset
    from .monitor_server import MonitorState, start_monitor_server
except ImportError:
    from camera import CameraConfig, LatestFrameCamera, source_value
    from detector import Detection
    from face_detector import YuNetFaceDetector
    from face_tracker import FaceTargetTracker, OffsetSmoother, calculate_offset
    from monitor_server import MonitorState, start_monitor_server


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run read-only face tracking on Orange Pi."
    )
    parser.add_argument(
        "--source",
        default=os.environ.get(
            "CAMERA_URL",
            "rtsp://127.0.0.1:8554/camera",
        ),
    )
    parser.add_argument(
        "--model",
        default="vision/models/face_detection_yunet_2023mar.onnx",
    )
    parser.add_argument("--confidence", type=float, default=0.6)
    parser.add_argument(
        "--inference-width",
        type=int,
        default=320,
        help="Resize frames to this width for YuNet inference; output stays full resolution.",
    )
    parser.add_argument(
        "--detect-every",
        type=int,
        default=2,
        help="Run YuNet once every N frames; intermediate frames reuse the tracked face.",
    )
    parser.add_argument("--output-dir", default="vision/outputs")
    parser.add_argument("--save-interval", type=float, default=5.0)
    parser.add_argument("--status-interval", type=float, default=5.0)
    parser.add_argument("--smooth-alpha", type=float, default=0.35)
    parser.add_argument("--monitor-host", default="0.0.0.0")
    parser.add_argument("--monitor-port", type=int, default=8090)
    parser.add_argument("--headless", action="store_true")
    return parser.parse_args()


def draw_face_tracking(
    frame,
    detection: Detection | None,
    offset,
    fps: float,
):
    annotated = frame.copy()
    height, width = frame.shape[:2]
    center = (width // 2, height // 2)
    cv2.drawMarker(
        annotated,
        center,
        (255, 255, 255),
        cv2.MARKER_CROSS,
        20,
        1,
    )
    if detection is not None and offset is not None:
        x, y, box_width, box_height = detection.bbox
        cv2.rectangle(
            annotated,
            (x, y),
            (x + box_width, y + box_height),
            (0, 220, 0),
            2,
        )
        face_center = offset.face_center
        cv2.line(annotated, center, face_center, (255, 190, 0), 2)
        cv2.circle(annotated, face_center, 5, (0, 0, 255), -1)
        label = (
            f"face {detection.confidence:.2f} "
            f"dx={offset.offset_px[0]} dy={offset.offset_px[1]}"
        )
        cv2.putText(
            annotated,
            label,
            (10, 24),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.55,
            (0, 220, 0),
            2,
            cv2.LINE_AA,
        )
    else:
        cv2.putText(
            annotated,
            "no face",
            (10, 24),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.65,
            (0, 0, 255),
            2,
            cv2.LINE_AA,
        )
    cv2.putText(
        annotated,
        f"FPS {fps:.1f}",
        (10, height - 12),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.55,
        (255, 255, 255),
        2,
        cv2.LINE_AA,
    )
    return annotated


def resize_for_inference(
    frame,
    inference_width: int,
):
    if inference_width <= 0 or frame.shape[1] <= inference_width:
        return frame, 1.0, 1.0
    scale = inference_width / frame.shape[1]
    inference_height = max(1, round(frame.shape[0] * scale))
    resized = cv2.resize(
        frame,
        (inference_width, inference_height),
        interpolation=cv2.INTER_AREA,
    )
    return resized, frame.shape[1] / inference_width, frame.shape[0] / inference_height


def scale_detections(
    detections: list[Detection],
    scale_x: float,
    scale_y: float,
) -> list[Detection]:
    scaled: list[Detection] = []
    for item in detections:
        x, y, width, height = item.bbox
        scaled.append(
            Detection(
                label=item.label,
                confidence=item.confidence,
                bbox=(
                    round(x * scale_x),
                    round(y * scale_y),
                    round(width * scale_x),
                    round(height * scale_y),
                ),
            )
        )
    return scaled


def main() -> int:
    args = parse_args()
    os.makedirs(args.output_dir, exist_ok=True)
    model_path = os.path.abspath(args.model)
    detector = YuNetFaceDetector(model_path, score_threshold=args.confidence)
    tracker = FaceTargetTracker()
    smoother = OffsetSmoother(args.smooth_alpha)
    camera = LatestFrameCamera(CameraConfig(source_value(args.source)))
    monitor_state = MonitorState()
    monitor_server, _ = start_monitor_server(
        monitor_state,
        args.monitor_host,
        args.monitor_port,
    )
    log_path = os.path.join(args.output_dir, "face_tracking.jsonl")
    latest_path = os.path.join(args.output_dir, "face_latest.json")
    latest_image_path = os.path.join(args.output_dir, "face_latest.jpg")
    last_saved = 0.0
    last_status = 0.0
    last_frame_time = time.monotonic()
    smoothed_fps = 0.0
    frame_count = 0

    print("人脸跟踪已启动：只读视频，不连接 STM32，不发送运动命令。")
    print(
        f"source={args.source}, model={model_path}, "
        f"monitor=http://0.0.0.0:{args.monitor_port}/",
        flush=True,
    )
    try:
        with open(log_path, "a", encoding="utf-8") as log_file:
            while True:
                ok, frame = camera.read()
                if not ok or frame is None:
                    continue

                now = time.monotonic()
                delta = max(now - last_frame_time, 1e-6)
                instant_fps = 1.0 / delta
                smoothed_fps = (
                    instant_fps
                    if smoothed_fps == 0
                    else smoothed_fps * 0.9 + instant_fps * 0.1
                )
                last_frame_time = now
                frame_count += 1

                if frame_count == 1 or frame_count % max(args.detect_every, 1) == 1:
                    inference_frame, scale_x, scale_y = resize_for_inference(
                        frame,
                        args.inference_width,
                    )
                    detections = scale_detections(
                        detector.detect(inference_frame),
                        scale_x,
                        scale_y,
                    )
                    selected = tracker.select(detections)
                else:
                    selected = tracker.previous
                raw_offset = (
                    calculate_offset(selected, frame.shape[1], frame.shape[0])
                    if selected is not None
                    else None
                )
                offset = smoother.update(raw_offset)
                annotated = draw_face_tracking(frame, selected, offset, smoothed_fps)
                event = {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "frame": frame_count,
                    "fps": round(smoothed_fps, 2),
                    "status": "face" if selected is not None else "no_face",
                    "bbox": selected.bbox if selected is not None else None,
                    "confidence": (
                        round(selected.confidence, 4)
                        if selected is not None
                        else None
                    ),
                    "face_center": offset.face_center if offset else None,
                    "screen_center": offset.screen_center if offset else None,
                    "offset_px": offset.offset_px if offset else None,
                    "offset_norm": (
                        tuple(round(value, 5) for value in offset.offset_norm)
                        if offset
                        else None
                    ),
                }
                log_file.write(json.dumps(event, ensure_ascii=False) + "\n")
                log_file.flush()

                encoded_ok, encoded = cv2.imencode(
                    ".jpg",
                    annotated,
                    [cv2.IMWRITE_JPEG_QUALITY, 75],
                )
                if encoded_ok:
                    monitor_state.update(encoded.tobytes(), event)

                if now - last_saved >= max(args.save_interval, 0.0):
                    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    cv2.imwrite(
                        os.path.join(args.output_dir, f"face_raw_{stamp}.jpg"),
                        frame,
                    )
                    cv2.imwrite(
                        os.path.join(args.output_dir, f"face_annotated_{stamp}.jpg"),
                        annotated,
                    )
                    cv2.imwrite(latest_image_path, annotated)
                    with open(latest_path, "w", encoding="utf-8") as latest_file:
                        json.dump(event, latest_file, ensure_ascii=False, indent=2)
                        latest_file.write("\n")
                    last_saved = now

                if now - last_status >= max(args.status_interval, 0.0):
                    if offset:
                        detail = (
                            f"dx={offset.offset_px[0]} "
                            f"dy={offset.offset_px[1]} "
                            f"confidence={selected.confidence:.2f}"
                        )
                    else:
                        detail = "no_face"
                    print(
                        f"[status] frames={frame_count} fps={smoothed_fps:.1f} {detail}",
                        flush=True,
                    )
                    last_status = now

                if not args.headless:
                    cv2.imshow("Orange Pi face tracking", annotated)
                    if cv2.waitKey(1) & 0xFF == ord("q"):
                        break
    except KeyboardInterrupt:
        print("\n收到退出信号。")
    finally:
        monitor_server.shutdown()
        monitor_server.server_close()
        camera.release()
        if not args.headless:
            cv2.destroyAllWindows()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
