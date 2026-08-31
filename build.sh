#!/bin/bash
# Panxcz Subway Surfers Tool - Build Script
# Builds native ELF binary using Android NDK cross-compiler

set -e

# NDK paths
if [ -n "$NDK_CC" ]; then
    CC="$NDK_CC"
else
    # Try common NDK locations
    ANDROID_NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk-r26b}"
    CC="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang"
fi

if [ ! -f "$CC" ]; then
    echo "ERROR: NDK compiler not found at $CC"
    echo "Set ANDROID_NDK_HOME or NDK_CC"
    exit 1
fi

echo "Using compiler: $CC"
$CC --version | head -1

# Build directory
mkdir -p build release

echo "=== Building subway_tool ==="

$CC \
    -O2 \
    -Wall -Wextra -Wno-unused-parameter \
    -D_GNU_SOURCE \
    -o build/subway_tool \
    src/subway_cheat.c \
    -lpthread \
    -static

chmod +x build/subway_tool

# Get binary size
SIZE=$(du -h build/subway_tool | cut -f1)
echo "Built: build/subway_tool ($SIZE)"

# Create wrapper script
cat > build/subway_tool.sh << 'WRAPPER'
#!/system/bin/sh
# Panxcz Subway Surfers Tool v1.0
# Usage: sh subway_tool.sh [options]

BASEDIR=$(dirname "$0")
TOOL="$BASEDIR/subway_tool"

# Check root
if [ "$(id -u)" != "0" ]; then
    echo "[!] Need root. Run:"
    echo "    su -c sh $0"
    exit 1
fi

# Make executable
chmod 777 "$TOOL"

# Run with args
exec "$TOOL" "$@"
WRAPPER

chmod +x build/subway_tool.sh

# Create README
cat > build/README.md << 'README'
# Panxcz Subway Surfers Tool v1.0

## Features
- **Score Multiplier** - 1x to 100x score
- **Coin Multiplier** - 1x to 100x coins
- **Speed Hack** - 1.0x to 10.0x speed
- **Jump Height** - 1.0x to 10.0x jump
- **Gravity** - 0.1x to 2.0x gravity
- **No Collision** - Pass through obstacles
- **Infinite Coins** - Max coins
- **Double Coins** - Double coin earn rate
- **Magnet Range** - 5x magnet range

## Requirements
- Root (su)
- Android ARM64 device

## Usage

### Interactive mode
```bash
su -c sh subway_tool.sh
```

### Command line mode
```bash
su -c ./subway_tool --all           # All cheats ON
su -c ./subway_tool --score 99      # 99x score
su -c ./subway_tool --speed 2.0     # 2x speed
su -c ./subway_tool --coins 50      # 50x coins
su -c ./subway_tool --no-collision  # No collision
```

### Options
| Flag | Description |
|------|-------------|
| `--score N` | Score multiplier (1-100) |
| `--coins N` | Coin multiplier (1-100) |
| `--speed F` | Speed multiplier (1.0-10.0) |
| `--jump F` | Jump height multiplier (1.0-10.0) |
| `--gravity F` | Gravity multiplier (0.1-2.0) |
| `--no-collision` | Toggle no collision |
| `--infinite-coins` | Toggle infinite coins |
| `--double-coins` | Toggle double coins |
| `--magnet` | Toggle extended magnet range |
| `--all` | Enable all cheats |

## Copyright
(c) 2025 Panxcz & Freebuff
Educational purposes only.
README

# Copy to release
cp build/subway_tool release/
cp build/subway_tool.sh release/
cp build/README.md release/

echo ""
echo "=== Build Complete ==="
echo "Binary:  build/subway_tool ($SIZE)"
echo "Wrapper: build/subway_tool.sh"
echo "Release: release/"
echo ""
echo "To test: adb push release/* /data/local/tmp/"
echo "         adb shell su -c sh /data/local/tmp/subway_tool.sh"
