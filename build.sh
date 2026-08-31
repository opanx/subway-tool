#!/bin/bash
# Panxcz Subway Surfers Tool v2.1 - Build Script
# Compiles ImGui + EGL overlay + cheat engine into single ELF
# Output: subway_tool.sh (self-contained)

set -e

# NDK
if [ -n "$NDK_CC" ]; then
    CC="$NDK_CC"
    # Derive other tools from CC path
    TOOLCHAIN_DIR=$(dirname "$CC")
    AR="${TOOLCHAIN_DIR}/llvm-ar"
    STRIP="${TOOLCHAIN_DIR}/llvm-strip"
else
    ANDROID_NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk-r26b}"
    TC="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64"
    CC="${TC}/bin/aarch64-linux-android33-clang"
    AR="${TC}/bin/llvm-ar"
    STRIP="${TC}/bin/llvm-strip"
fi

if [ ! -f "$CC" ]; then
    echo "ERROR: NDK not found at $CC"
    exit 1
fi

echo "Compiler: $CC"
$CC --version | head -1

# Flags
CFLAGS="-O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -std=c++17"
CFLAGS="$CFLAGS -DANDROID -DIMGUI_IMPL_ANDROID -DIMGUI_IMPL_OPENGL_ES3"
INCLUDES="-Isrc -Isrc/imgui"

mkdir -p build build/obj release

echo "=== Compiling ImGui core ==="

# Compile ImGui core
for f in imgui.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp; do
    echo "  $f"
    $CC $CFLAGS $INCLUDES -c "src/imgui/$f" -o "build/obj/${f%.cpp}.o" || exit 1
done

echo "=== Compiling Android backend ==="
$CC $CFLAGS $INCLUDES -c src/imgui_impl_android.cpp -o build/obj/imgui_impl_android.o

echo "=== Compiling OpenGL3 backend ==="
$CC $CFLAGS $INCLUDES -c src/imgui_impl_opengl3.cpp -o build/obj/imgui_impl_opengl3.o

echo "=== Compiling cheat engine ==="
$CC $CFLAGS $INCLUDES -c src/subway_cheat.c -o build/obj/subway_cheat.o

echo "=== Linking ==="
$CC \
    build/obj/*.o \
    -lEGL -lGLESv3 -llog -landroid -ldl \
    -o build/subway_tool

chmod +x build/subway_tool

# Strip
if [ -f "$STRIP" ]; then
    $STRIP --strip-all build/subway_tool
fi

SIZE=$(du -h build/subway_tool | cut -f1)
echo "Built: build/subway_tool ($SIZE)"

# Create self-extracting .sh
cat > release/subway_tool.sh << 'HEADER'
#!/system/bin/sh
# ============================================
# 🎮 Panxcz Subway Surfers Tool v2.1
# By Panxcz & Freebuff
# Copyright (c) 2025
# ============================================
# Interactive ImGui overlay menu
# 3-finger tap = toggle menu
# ============================================

BASEDIR=$(cd "$(dirname "$0")" && pwd)
BIN="$BASEDIR/.panxcz_subway"

# Extract
if [ ! -f "$BIN" ] || [ "$(head -c 4 "$BIN" 2>/dev/null)" = "#!/b" ]; then
    echo "[*] Extracting Panxcz Subway Tool..."
    sed -n '/^__BIN_BELOW__$/,$ p' "$0" | tail -n +2 > "$BIN"
    chmod 777 "$BIN"
fi

# Root check
if [ "$(id -u)" != "0" ]; then
    echo "[!] Need root: su -c sh $0"
    exit 1
fi

echo "[*] Starting Panxcz Subway Tool..."
echo "[*] 3-finger tap = toggle overlay menu"
exec "$BIN" "$@"

__BIN_BELOW__
HEADER

base64 build/subway_tool >> release/subway_tool.sh
chmod +x release/subway_tool.sh

FINAL=$(du -h release/subway_tool.sh | cut -f1)
echo ""
echo "=== Build Complete ==="
echo "Output: release/subway_tool.sh ($FINAL)"
echo ""
echo "Test:"
echo "  adb push release/subway_tool.sh /data/local/tmp/"
echo "  adb shell su -c sh /data/local/tmp/subway_tool.sh"
