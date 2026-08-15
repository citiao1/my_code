#!/usr/bin/env bash
set -euo pipefail

service_name="mecanum-console.service"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "$script_dir/.." && pwd)"
template_path="$script_dir/$service_name.in"
unit_path="/etc/systemd/system/$service_name"

if [[ ${EUID:-$(id -u)} -ne 0 ]]; then
  echo "Run this installer with sudo:"
  echo "  sudo bash $script_dir/install-service.sh"
  exit 1
fi

service_user="${SUDO_USER:-}"
if [[ -z "$service_user" || "$service_user" == "root" ]]; then
  echo "Unable to determine the normal login user."
  echo "Run this script from the orangepi account with sudo."
  exit 1
fi

service_home="$(getent passwd "$service_user" | cut -d: -f6)"
service_group="$(id -gn "$service_user")"

if [[ -z "$service_home" || ! -f "$project_dir/orange-pi/start.sh" ]]; then
  echo "Invalid user home or project directory."
  exit 1
fi

if [[ "$project_dir" == *" "* || "$service_home" == *" "* ]]; then
  echo "Paths containing spaces are not supported by this installer."
  exit 1
fi

runtime_unit="$(mktemp)"
trap 'rm -f "$runtime_unit"' EXIT

sed \
  -e "s|@SERVICE_USER@|$service_user|g" \
  -e "s|@SERVICE_GROUP@|$service_group|g" \
  -e "s|@SERVICE_HOME@|$service_home|g" \
  -e "s|@PROJECT_DIR@|$project_dir|g" \
  "$template_path" > "$runtime_unit"

systemctl stop "$service_name" 2>/dev/null || true
install -m 0644 "$runtime_unit" "$unit_path"
systemctl daemon-reload
systemctl enable --now "$service_name"

echo
echo "Installed and started: $service_name"
echo "Check status: sudo systemctl status $service_name"
echo "Follow logs:  journalctl -u $service_name -f"
echo "Stop service: sudo systemctl stop $service_name"
