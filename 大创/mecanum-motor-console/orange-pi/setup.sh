#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
venv_dir="${MECANUM_CONSOLE_VENV:-$HOME/.venvs/mecanum-console}"

if ! python3 -m venv "$venv_dir"; then
  echo "Unable to create the Python environment."
  echo "Install it first: sudo apt install -y python3-venv"
  exit 1
fi

"$venv_dir/bin/python" -m pip install --upgrade pip
"$venv_dir/bin/python" -m pip install -r "$script_dir/requirements.txt"

echo "Setup complete: $venv_dir"

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "Missing FFmpeg: sudo apt install -y ffmpeg"
fi

media_dir="${MEDIAMTX_DIR:-$HOME/mediamtx}"
if [[ ! -x "$media_dir/mediamtx" || ! -f "$media_dir/mediamtx.yml" ]]; then
  echo "Missing MediaMTX under: $media_dir"
  echo "Install the linux_arm64 release before running orange-pi/start.sh."
fi
