#!/bin/bash
# Panxcz Subway Tool v2.1 - Build Script
# Builds: injector (ELF) + liboverlay.so

set -e

if [ -n "$NDK_CC" ]; then
    CC="$NDK_CC"
    TC=$(dirname "$CC")
    STRIP="${TC}/llvm-strip"
    TARGET=android-33
else
    ANDROID_NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk-r26b}"
    TC="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64"
    CC="${TC}/bin/aarch64-linux-android33-clang"
    STRIP="${TC}/bin/llvm-strip"
    TARGET=android-33
fi

CFLAGS="-O2 -Wall -Wno-unused-parameter"
CFLAGS_CPP="$CFLAGS -std=c++17 -DIMGUI_IMPL_OPENGL_ES3"
INCLUDES="-Isrc/imgui -Isrc/overlay"

echo "=== Building liboverlay.so (injected into game) ==="
$CC $CFLAGS_CPP $INCLUDES -shared -fPIC \
    -o build/liboverlay.so \
    src/imgui/imgui.cpp \
    src/imgui/imgui_draw.cpp \
    src/imgui/imgui_widgets.cpp \
    src/imgui/imgui_tables.cpp \
    src/imgui_impl_opengl3.cpp \
    src/overlay/overlay.cpp \
    -lEGL -lGLESv3 -llog -landroid -lc++ -lm

echo "=== Building injector (standalone ELF) ==="
$CC $CFLAGS -DANDROID \
    -o build/subway_tool \
    src/injector/injector.cpp \
    -llog

chmod +x build/subway_tool
chmod 755 build/liboverlay.so

$STRIP --strip-all build/subway_tool 2>/dev/null || true
$STRIP --strip-all build/liboverlay.so 2>/dev/null || true

echo ""
echo "=== Build Complete ==="
echo "Injector: build/subway_tool ($(du -h build/subway_tool | cut -f1))"
echo "Overlay:  build/liboverlay.so ($(du -h build/liboverlay.so | cut -f1))"
