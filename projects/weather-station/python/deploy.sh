#!/bin/sh
# Pulls the latest code, rebuilds the service environments, and installs and restarts the systemd units.
# Run on the Pi as the user the services run as. The script calls sudo where it needs it.
set -eu

TIMERS="weather-digest.timer weather-alarm.timer"
BOOT_UNITS="weather-boot.service"
UNITS="weather-sensors.service weather-web.service weather-digest.service weather-alarm.service $TIMERS $BOOT_UNITS"
TIME_WAIT_UNIT=systemd-time-wait-sync.service
TIME_WAIT_DROPIN_DIR=/etc/systemd/system/systemd-time-wait-sync.service.d
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

  # A first install turns each timer and boot unit on. Later deploys keep whatever state it was left in.
  enabled_units=""
  for unit in $TIMERS $BOOT_UNITS; do
    if [ ! -f "/etc/systemd/system/$unit" ] || systemctl is-enabled --quiet "$unit"; then
      enabled_units="$enabled_units $unit"
    fi
  done

  sudo systemctl stop $TIMERS weather-web.service weather-sensors.service 2>/dev/null || true

  ./init.sh services

  for unit in $UNITS; do
    sed -e "s|^User=pi\$|User=$service_user|" \
        -e "s|$UNIT_TEMPLATE_DIR|$python_dir|g" \
        "../deploy/$unit" | sudo tee "/etc/systemd/system/$unit" >/dev/null
  done
  sudo mkdir -p "$TIME_WAIT_DROPIN_DIR"
  sudo cp ../deploy/systemd-time-wait-sync.service.d/timeout.conf "$TIME_WAIT_DROPIN_DIR/timeout.conf"
  sudo systemctl daemon-reload
  sudo systemctl enable "$TIME_WAIT_UNIT"

  sudo systemctl enable --now weather-sensors.service
  sudo systemctl enable --now weather-web.service
  for timer in $TIMERS; do
    case " $enabled_units " in
      *" $timer "*) sudo systemctl enable --now "$timer" ;;
      *) echo "Left $timer disabled" ;;
    esac
  done
  for unit in $BOOT_UNITS; do
    case " $enabled_units " in
      *" $unit "*) sudo systemctl enable "$unit" ;;
      *) echo "Left $unit disabled" ;;
    esac
  done
  systemctl --no-pager status weather-sensors.service weather-web.service $TIMERS
}

# git pull can rewrite this file mid-run. sh parses main whole and reads this line before it runs.
main "$@"; exit
