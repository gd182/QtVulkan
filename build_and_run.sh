#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
APP="$BUILD_DIR/appQtVulkan.app"
VULKAN_SETUP="$HOME/VulkanSDK/1.4.341.1/setup-env.sh"

# Активировать Vulkan SDK
if [[ -f "$VULKAN_SETUP" ]]; then
    source "$VULKAN_SETUP"
else
    echo "Warning: Vulkan SDK setup script not found at $VULKAN_SETUP"
fi

echo "[1/3] Configuring..."
cmake -B "$BUILD_DIR" \
      -DCMAKE_PREFIX_PATH=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Debug \
      -S "$SCRIPT_DIR"

echo "[2/3] Building..."
cmake --build "$BUILD_DIR" --parallel

echo "[3/3] Running..."
echo "----------------------------------------"
"$APP/Contents/MacOS/appQtVulkan"
EXIT_CODE=$?
echo "----------------------------------------"
echo "Exit code: $EXIT_CODE"
