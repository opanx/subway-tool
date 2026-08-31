# 🎮 Panxcz Subway Surfers Tool

> **External ELF Binary Cheat Tool for Subway Surfers**
> Copyright (c) 2025 Panxcz & Freebuff

## ⚡ Features

| Feature | Description |
|---------|-------------|
| 🏆 **Score Multiplier** | 1x - 100x score multiplier |
| 💰 **Coin Multiplier** | 1x - 100x coin multiplier |
| 🏃 **Speed Hack** | 1.0x - 10.0x game speed |
| 🦘 **Jump Height** | 1.0x - 10.0x jump height |
| 🌊 **Gravity** | 0.1x - 2.0x gravity control |
| 🚫 **No Collision** | Pass through all obstacles |
| 💎 **Infinite Coins** | Max coins during run |
| 🪙 **Double Coins** | Double coin earn rate |
| 🧲 **Magnet Range** | 5x magnet pickup range |

## 📋 Requirements

- Android ARM64 device
- Root access (su)
- Subway Surfers installed

## 🚀 Usage

### Download
1. Go to [Releases](../../releases)
2. Download the latest `subway_tool` + `subway_tool.sh`
3. Push to device:
```bash
adb push subway_tool subway_tool.sh /data/local/tmp/
adb shell su -c "chmod 777 /data/local/tmp/*"
```

### Interactive Mode
```bash
adb shell su -c sh /data/local/tmp/subway_tool.sh
```

### Command Line Mode
```bash
# All cheats enabled
adb shell su -c "/data/local/tmp/subway_tool --all"

# Specific cheats
adb shell su -c "/data/local/tmp/subway_tool --score 99 --coins 50 --speed 2.0"
```

### Options
| Flag | Default | Range |
|------|---------|-------|
| `--score N` | 1 | 1-100 |
| `--coins N` | 1 | 1-100 |
| `--speed F` | 1.0 | 1.0-10.0 |
| `--jump F` | 1.0 | 1.0-10.0 |
| `--gravity F` | 1.0 | 0.1-2.0 |
| `--no-collision` | OFF | Toggle |
| `--infinite-coins` | OFF | Toggle |
| `--double-coins` | OFF | Toggle |
| `--magnet` | OFF | Toggle |
| `--all` | - | Enable all |

## 🔧 How It Works

1. **Process Scan** - Finds Subway Surfers process via `/proc/*/cmdline`
2. **Memory Scan** - Scans for `CoreRunnerManager` instance pattern
3. **Instance Validation** - Verifies valid pointers + float values
4. **Live Patching** - Writes cheat values directly to game memory every 100ms

### Memory Structure (from IL2CPP dump)
```
CoreRunnerManager
├── +0x10: bool IsInActiveRun
├── +0x18: RunSessionData*
│   ├── +0x18: SafeFloat distance
│   ├── +0x28: SafeInt coins
│   ├── +0x48: int bonusCoins
│   └── +0x50: RunVariableGroup points
└── +0x20: CoreRunMultiplier*
    ├── +0x10: int boosterMultiplier
    ├── +0x18: int eventScoreMultiplier
    ├── +0x1c: bool doubleScoreActive
    └── +0x20: int totalMultiplier
```

## ⚠️ Disclaimer

**EDUCATIONAL PURPOSES ONLY**

This tool is for learning reverse engineering and Android security research. Do not use for cheating in online games. The author is not responsible for any consequences.

## 📝 Build from Source

### GitHub Actions (Recommended)
1. Push code to GitHub
2. Go to Actions → Build subway_tool → Run workflow
3. Download artifact from completed run

### Local Build (requires NDK)
```bash
export ANDROID_NDK_HOME=/path/to/ndk-r26b
bash build.sh
```

## 🐙 Credits

- **Panxcz** - Developer
- **Freebuff** - AI Assistant
- Built with Android NDK + AArch64 cross-compiler

---
*Made with 🐙 by Panxcz & Freebuff*
