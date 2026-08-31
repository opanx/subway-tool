#!/bin/bash
# Panxcz Subway Surfers Tool v2.1 - Build Script

set -e

# NDK
if [ -n "$NDK_CC" ]; then
    CC="$NDK_CC"
    TOOLCHAIN_DIR=$(dirname "$CC")
    STRIP="${TOOLCHAIN_DIR}/llvm-strip"
else
    ANDROID_NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk-r26b}"
    TC="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64"
    CC="${TC}/bin/aarch64-linux-android33-clang"
    STRIP="${TC}/bin/llvm-strip"
fi

echo "Compiler: $CC"
$CC --version | head -1

CFLAGS_C="-O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -DANDROID"
CFLAGS_CPP="-O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -std=c++17 -DANDROID -DIMGUI_IMPL_ANDROID -DIMGUI_IMPL_OPENGL_ES3"
INCLUDES="-Isrc -Isrc/imgui"

mkdir -p build/obj release

echo "=== Compiling ImGui core (C++) ==="
for f in imgui.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp; do
    echo "  $f"
    $CC $CFLAGS_CPP $INCLUDES -c "src/imgui/$f" -o "build/obj/${f%.cpp}.o" || exit 1
done

echo "=== Compiling Android backend (C++) ==="
$CC $CFLAGS_CPP $INCLUDES -c src/imgui_impl_android.cpp -o build/obj/imgui_impl_android.o

echo "=== Compiling OpenGL3 backend (C++) ==="
$CC $CFLAGS_CPP $INCLUDES -c src/imgui_impl_opengl3.cpp -o build/obj/imgui_impl_opengl3.o

echo "=== Compiling cheat engine (C++) ==="
$CC $CFLAGS_CPP $INCLUDES -c src/subway_cheat.cpp -o build/obj/subway_cheat.o

echo "=== Linking ==="
$CC \
    build/obj/*.o \
    -lEGL -lGLESv3 -llog -landroid -ldl \
    -lc++ -lm \
    -o build/subway_tool

chmod +x build/subway_tool

# Strip
if [ -f "$STRIP" ]; then
    $STRIP --strip-all build/subway_tool
fi

SIZE=$(du -h build/subway_tool | cut -f1)
echo "Built: build/subway_tool ($SIZE)"

# Create wrapper .sh (expects binary in same dir)
cat > release/subway_tool.sh << 'EOF'
#!/system/bin/sh
# 🎮 Panxcz Subway Surfers Tool v2.1
# By Panxcz & Freebuff
# 3-finger tap = toggle overlay menu

BASEDIR=$(cd "$(dirname "$0")" && pwd)
BIN="$BASEDIR/subway_tool"

if [ ! -f "$BIN" ]; then
    echo "[!] Binary not found: $BIN"
    echo "[!] Download subway_tool binary first"
    exit 1
fi

if [ "$(id -u)" != "0" ]; then
    echo "[!] Need root: su -c sh $0"
    exit 1
fi

chmod 777 "$BIN"
echo "[*] Starting Panxcz Subway Tool v2.1..."
echo "[*] 3-finger tap = toggle overlay"
exec "$BIN" "$@"
EOF

chmod +x release/subway_tool.sh

echo ""
echo "=== Build Complete ==="
echo "Binary:  build/subway_tool ($SIZE)"
echo "Wrapper: release/subway_tool.sh"
echo ""
echo "Test:"
echo "  adb push build/subway_tool release/subway_tool.sh /data/local/tmp/"
echo "  adb shell su -c sh /data/local/tmp/subway_tool.sh"
