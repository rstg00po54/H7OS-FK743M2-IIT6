#!/usr/bin/env bash
# Install or repair RT-Thread Env using public GitHub repositories only.
# Avoids automatic Gitee mirror selection in the upstream touch_env.sh script.
# This script never deletes a pre-existing nonempty directory.
set -euo pipefail

ENV_ROOT="${ENV_ROOT:-$HOME/.env}"
mkdir -p "$ENV_ROOT/packages" "$ENV_ROOT/tools" "$ENV_ROOT/local_pkgs"

clone_or_repair() {
  local repo="$1" dst="$2"
  if [[ -d "$dst/.git" ]]; then
    echo "Found Git checkout: $dst"
    git -C "$dst" remote set-url origin "$repo"
    return
  fi
  if [[ -e "$dst" ]]; then
    if [[ -d "$dst" && -z "$(ls -A "$dst")" ]]; then
      rmdir "$dst"
    else
      echo "Cannot replace a nonempty non-Git directory: $dst" >&2
      echo "Please inspect it and rename manually before retrying." >&2
      exit 1
    fi
  fi
  echo "Cloning $repo into $dst..."
  GIT_TERMINAL_PROMPT=0 git clone --depth=1 "$repo" "$dst"
}

clone_or_repair "https://github.com/RT-Thread/packages.git" "$ENV_ROOT/packages/packages"
clone_or_repair "https://github.com/RT-Thread/sdk.git" "$ENV_ROOT/packages/sdk"
clone_or_repair "https://github.com/RT-Thread/env.git" "$ENV_ROOT/tools/scripts"

printf 'source "$PKGS_DIR/packages/Kconfig"\n' > "$ENV_ROOT/packages/Kconfig"
cp "$ENV_ROOT/tools/scripts/env.sh" "$ENV_ROOT/env.sh"

echo
echo "RT-Thread Env cloned from GitHub."
echo "Install the Python venv prerequisite if needed:"
echo "  sudo apt install python3-venv"
echo "Activate in your shell:"
echo "  source \"$ENV_ROOT/env.sh\""
echo "Then run H7OS's dependency helper and build:"
echo "  bash scripts/setup_deps.sh"
echo "  bash scripts/build.sh"
