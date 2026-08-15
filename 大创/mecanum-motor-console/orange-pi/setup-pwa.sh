#!/usr/bin/env bash
set -euo pipefail

github_page="${GITHUB_PAGE_URL:-https://citiao1.github.io/my_code/}"

if ! command -v tailscale >/dev/null 2>&1; then
  echo "Tailscale is not installed."
  exit 1
fi

if ! tailscale ip -4 >/dev/null 2>&1; then
  echo "Tailscale is not logged in. Run: sudo tailscale up"
  exit 1
fi

if ! tailscale_dns_name="$({ tailscale status --json || true; } | python3 -c '
import json
import sys

try:
    print(json.load(sys.stdin)["Self"]["DNSName"].rstrip("."))
except (KeyError, TypeError, ValueError):
    raise SystemExit(1)
')"; then
  echo "Unable to read the Tailscale DNS name."
  exit 1
fi

tailscale serve --bg --https=443 http://127.0.0.1:8088
tailscale serve --https=8443 off 2>/dev/null || true
tailscale serve --https=10000 off 2>/dev/null || true
tailscale serve --https=443 --set-path=/ws off 2>/dev/null || true
tailscale serve --https=443 --set-path=/camera off 2>/dev/null || true
tailscale serve --https=443 --set-path=/camera/ off 2>/dev/null || true
tailscale serve --bg --https=443 --set-path=/ws http://127.0.0.1:8766
tailscale serve --bg --https=443 --set-path=/camera/ http://127.0.0.1:8889

target_url="https://$tailscale_dns_name"
github_remote_url="${github_page}?target=${target_url}"

echo
tailscale serve status
echo
echo "Orange Pi HTTPS page: $target_url"
echo "GitHub remote page:    $github_remote_url"
echo
echo "Open the GitHub remote page once on the phone, then use the browser menu"
echo "to install it or add it to the home screen."
