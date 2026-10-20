#!/bin/sh
# Rebuilds the weather station virtual environments. Runs on macOS and Raspberry Pi OS.
set -eu

cd "$(dirname "$0")"

ALL_SERVICES="sensors web notifications"

usage() {
  echo "Usage: ./init.sh dev" >&2
  echo "       ./init.sh services [sensors] [web] [notifications]" >&2
  exit 1
}

if command -v uv >/dev/null 2>&1; then
  USE_UV=1
else
  USE_UV=0
  if ! python3 -c 'import sys; sys.exit(sys.version_info < (3, 12))' 2>/dev/null; then
    echo "Python 3.12 or newer is required. Found: $(python3 --version 2>&1)" >&2
    exit 1
  fi
fi

create_venv() {
  rm -rf "$1"
  if [ "$USE_UV" = 1 ]; then
    uv venv -q "$1"
  else
    python3 -m venv "$1"
  fi
}

install_into() {
  venv=$1
  shift
  if [ "$USE_UV" = 1 ]; then
    uv pip install -q --python "$venv/bin/python" "$@"
  else
    "$venv/bin/python" -m pip install -q --disable-pip-version-check "$@"
  fi
}

init_dev() {
  create_venv .venv
  install_into .venv -r requirements-dev.txt
  echo "Built .venv ($(.venv/bin/python --version))"
}

init_services() {
  [ $# -eq 0 ] && set -- $ALL_SERVICES
  for service in "$@"; do
    case " $ALL_SERVICES " in
      *" $service "*) ;;
      *) echo "Unknown service: $service" >&2; usage ;;
    esac
  done
  for service in "$@"; do
    create_venv ".venv-$service"
    install_into ".venv-$service" ./shared "./$service"
    echo "Built .venv-$service ($(".venv-$service/bin/python" --version))"
  done
}

copy_env_template() {
  if [ -f .env ]; then
    echo "Kept existing .env"
  else
    cp .env.example .env
    echo "Created .env from .env.example. Add SMTP credentials before starting notifications."
  fi
}

[ $# -ge 1 ] || usage
mode=$1
shift

case "$mode" in
  dev) init_dev ;;
  services)
    init_services "$@"
    echo "Next: install the units in ../deploy. See docs/getting-started/setting-up-web-server."
    ;;
  *) usage ;;
esac

copy_env_template
