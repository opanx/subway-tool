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
