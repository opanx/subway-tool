#!/bin/bash
# Panxcz Subway Surfers Tool - Build Script
# Outputs: subway_tool.sh (self-contained wrapper)

set -e

# NDK paths
if [ -n "$NDK_CC" ]; then
    CC="$NDK_CC"
else
    ANDROID_NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk-r26b}"
    CC="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang"
fi

if [ ! -f "$CC" ]; then
    echo "ERROR: NDK compiler not found at $CC"
    exit 1
fi

echo "Using: $CC"
$CC --version | head -1

mkdir -p build release

echo "=== Building subway_tool ==="

# Compile
$CC \
    -O2 \
    -Wall -Wextra -Wno-unused-parameter \
    -o build/subway_tool \
    src/subway_cheat.c

chmod +x build/subway_tool

SIZE=$(du -h build/subway_tool | cut -f1)
echo "Built: build/subway_tool ($SIZE)"

# Create self-extracting .sh wrapper
cat > release/subway_tool.sh << 'HEADER'
#!/system/bin/sh
# ============================================
# 🎮 Panxcz Subway Surfers Tool v2.0
# By Panxcz & Freebuff
# Copyright (c) 2025
# ============================================
# Usage:
#   sh subway_tool.sh           (interactive)
#   sh subway_tool.sh --all     (all cheats)
#   sh subway_tool.sh --help    (show help)
# ============================================

BASEDIR=$(cd "$(dirname "$0")" && pwd)
BIN="$BASEDIR/.subway_tool_bin"

# Extract embedded binary
if [ ! -f "$BIN" ]; then
    echo "[*] Extracting binary..."
    sed -n '/^__BIN_BELOW__$/,$ p' "$0" | tail -n +2 > "$BIN"
    chmod 777 "$BIN"
fi

# Check root
if [ "$(id -u)" != "0" ]; then
    echo "[!] Need root. Run: su -c sh $0"
    exit 1
fi

# Run
exec "$BIN" "$@"

# Binary data below this line (do not edit)
__BIN_BELOW__
HEADER

# Append binary as base64 (safe for shell)
base64 build/subway_tool >> release/subway_tool.sh

chmod +x release/subway_tool.sh

FINAL_SIZE=$(du -h release/subway_tool.sh | cut -f1)
echo ""
echo "=== Done ==="
echo "Output: release/subway_tool.sh ($FINAL_SIZE)"
echo ""
echo "Test on device:"
echo "  adb push release/subway_tool.sh /data/local/tmp/"
echo "  adb shell su -c sh /data/local/tmp/subway_tool.sh"
