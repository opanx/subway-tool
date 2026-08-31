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
