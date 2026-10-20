#!/bin/sh
# Pulls the latest code, rebuilds the service environments, and installs and restarts the systemd units.
# Run on the Pi as the user the services run as. The script calls sudo where it needs it.
set -eu

UNITS="weather-sensors.service weather-web.service weather-digest.service weather-digest.timer"
UNIT_TEMPLATE_DIR=/home/pi/embedded/projects/weather-station/python

main() {
  cd "$(dirname "$0")"
  python_dir=$(pwd)
  service_user=$(id -un)

  if [ "$(id -u)" -eq 0 ]; then
    echo "Run as the service user, not root." >&2
    exit 1
  fi
  if ! command -v systemctl >/dev/null 2>&1; then
    echo "systemctl not found. deploy.sh runs on the Pi only." >&2
    exit 1
  fi

  git pull --ff-only

  # A first install turns the digest on. Later deploys keep whatever state it was left in.
  digest_enabled=yes
  if [ -f /etc/systemd/system/weather-digest.timer ] && ! systemctl is-enabled --quiet weather-digest.timer; then
    digest_enabled=no
  fi

  sudo systemctl stop weather-digest.timer weather-web.service weather-sensors.service 2>/dev/null || true

  ./init.sh services

  for unit in $UNITS; do
    sed -e "s|^User=pi\$|User=$service_user|" \
        -e "s|$UNIT_TEMPLATE_DIR|$python_dir|g" \
        "../deploy/$unit" | sudo tee "/etc/systemd/system/$unit" >/dev/null
  done
  sudo systemctl daemon-reload

  sudo systemctl enable --now weather-sensors.service
  sudo systemctl enable --now weather-web.service
  if [ "$digest_enabled" = yes ]; then
    sudo systemctl enable --now weather-digest.timer
  else
    echo "Left weather-digest.timer disabled"
  fi
  systemctl --no-pager status weather-sensors.service weather-web.service weather-digest.timer
}

# git pull can rewrite this file mid-run. sh parses main whole and reads this line before it runs.
main "$@"; exit
