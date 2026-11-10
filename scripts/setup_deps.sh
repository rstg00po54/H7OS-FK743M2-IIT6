#!/usr/bin/env bash
# Fetch HAL/CMSIS package dependencies selected by the generated BSP .config.
# Uses the official RT-Thread Env package manager; no root privileges required.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BSP="$ROOT/third_party/rt-thread/bsp/stm32/stm32h743-fk743m2-iit6"

if [[ ! -f "$BSP/.config" ]]; then
    echo "Missing generated BSP: run python3 scripts/prepare.py" >&2
    exit 1
fi

if ! command -v pkgs >/dev/null 2>&1; then
    if [[ -f "$HOME/.env/env.sh" ]]; then
        # RT-Thread's official Env activation script installs helper functions
        # into this Bash subprocess. It does not change the user's shell.
        # shellcheck disable=SC1091
        source "$HOME/.env/env.sh"
    fi
fi

if ! command -v pkgs >/dev/null 2>&1; then
    echo "RT-Thread Env is not installed or activated." >&2
    echo "Official Ubuntu installation:" >&2
    echo "  cd ~" >&2
    echo "  wget https://raw.githubusercontent.com/RT-Thread/env/master/install_ubuntu.sh" >&2
    echo "  bash install_ubuntu.sh" >&2
    echo "  source ~/.env/env.sh" >&2
    echo "Then run: bash scripts/setup_deps.sh" >&2
    exit 1
fi

cd "$BSP"
echo "Downloading dependencies from generated BSP .config in: $BSP"
pkgs --update
echo
echo "Dependency installation requested. Run: bash scripts/build.sh"
