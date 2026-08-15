#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "$script_dir/.." && pwd)"
venv_dir="${MECANUM_CONSOLE_VENV:-$HOME/.venvs/mecanum-console}"
serial_port="${SERIAL_PORT:-/dev/ttyS5}"
serial_baud="${SERIAL_BAUD:-115200}"
listen_host="${CONSOLE_HOST:-0.0.0.0}"
http_port="${HTTP_PORT:-8088}"
ws_port="${WS_PORT:-8766}"
camera_device="${CAMERA_DEVICE:-/dev/video1}"
camera_resolution="${CAMERA_RESOLUTION:-320x240}"
camera_fps="${CAMERA_FPS:-15}"
camera_bitrate="${CAMERA_BITRATE:-400k}"
camera_buffer_size="${CAMERA_BUFFER_SIZE:-80k}"
media_dir="${MEDIAMTX_DIR:-$HOME/mediamtx}"
media_bin="${MEDIAMTX_BIN:-$media_dir/mediamtx}"
media_config="${MEDIAMTX_CONFIG:-$media_dir/mediamtx.yml}"
media_rtsp_url="${MEDIAMTX_RTSP_URL:-rtsp://127.0.0.1:8554/camera}"

if [[ ! -x "$venv_dir/bin/python" ]]; then
  echo "Python environment not found: $venv_dir"
  echo "Run first: bash $script_dir/setup.sh"
  exit 1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "FFmpeg not found. Install it first: sudo apt install -y ffmpeg"
  exit 1
fi

if [[ ! -x "$media_bin" || ! -f "$media_config" ]]; then
  echo "MediaMTX not found under: $media_dir"
  echo "Expected: $media_bin and $media_config"
  exit 1
fi

runtime_media_config="$(mktemp)"
cp "$media_config" "$runtime_media_config"
if command -v tailscale >/dev/null 2>&1; then
  tailscale_ip="$(tailscale ip -4 2>/dev/null | head -n 1 || true)"
  if [[ -n "$tailscale_ip" ]]; then
    sed -i "s/^webrtcAdditionalHosts:.*/webrtcAdditionalHosts: [$tailscale_ip]/" "$runtime_media_config"
  fi
fi

cleanup() {
  trap - EXIT INT TERM
  [[ -n "${http_pid:-}" ]] && kill "$http_pid" 2>/dev/null || true
  [[ -n "${bridge_pid:-}" ]] && kill "$bridge_pid" 2>/dev/null || true
  [[ -n "${ffmpeg_pid:-}" ]] && kill "$ffmpeg_pid" 2>/dev/null || true
  [[ -n "${media_pid:-}" ]] && kill "$media_pid" 2>/dev/null || true
  wait "${http_pid:-}" "${bridge_pid:-}" "${ffmpeg_pid:-}" "${media_pid:-}" 2>/dev/null || true
  rm -f "$runtime_media_config"
}
trap cleanup EXIT INT TERM

"$venv_dir/bin/python" -m http.server "$http_port" \
  --bind "$listen_host" --directory "$project_dir" &
http_pid=$!

"$venv_dir/bin/python" "$project_dir/bridge/motor_vofa_bridge.py" \
  --transport serial \
  --port "$serial_port" \
  --baud "$serial_baud" \
  --ws-host "$listen_host" \
  --ws-port "$ws_port" &
bridge_pid=$!

"$media_bin" "$runtime_media_config" &
media_pid=$!

sleep 1

ffmpeg -nostdin -hide_banner -loglevel warning \
  -fflags nobuffer -flags low_delay -thread_queue_size 1 \
  -f v4l2 -input_format mjpeg \
  -video_size "$camera_resolution" -framerate "$camera_fps" \
  -i "$camera_device" -an \
  -c:v libx264 -preset ultrafast -tune zerolatency \
  -pix_fmt yuv420p -profile:v baseline -bf 0 \
  -g "$camera_fps" -keyint_min "$camera_fps" -sc_threshold 0 \
  -b:v "$camera_bitrate" -maxrate "$camera_bitrate" -bufsize "$camera_buffer_size" \
  -f rtsp -rtsp_transport tcp "$media_rtsp_url" &
ffmpeg_pid=$!

echo "Web console: http://<orange-pi-ip>:$http_port"
echo "UART bridge: $serial_port @ $serial_baud"
echo "WebRTC camera: http://<orange-pi-ip>:8889/camera"
echo "Press Ctrl+C to stop all services."

wait -n "$http_pid" "$bridge_pid" "$media_pid" "$ffmpeg_pid"
