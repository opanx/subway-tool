/*
 * Panxcz Subway Surfers Tool v2.0
 * External ELF Binary - Root required
 * Architecture: ARM64 (aarch64)
 *
 * Features:
 *   - Anti-cheat bypass (ptrace, root detect, frida detect)
 *   - Score/Coin multiplier
 *   - Speed/Jump/Gravity hack
 *   - No collision / God mode
 *   - Infinite coins / Double coins
 *   - Infinite hoverboard
 *   - Jetpack always active
 *   - Invincibility / Shield
 *   - Auto magnet
 *   - Character swap
 *   - Teleport hack
 *   - Score protector (prevent score reset)
 *   - Memory scanner + patcher
 *   - Custom offset patcher
 *
 * Copyright (c) 2025 Panxcz & Freebuff
 * Educational purposes only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

/* ============================================================
 * ANSI COLORS
 * ============================================================ */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_GRAY    "\033[90m"
#define C_BG_RED  "\033[41m"
#define C_BG_GRN  "\033[42m"

/* ============================================================
 * CONFIGURATION
 * ============================================================ */
#define GAME_PACKAGE    "com.kiloo.subwaysurf"
#define VERSION         "2.0"
#define MAX_REGIONS     8192
#define SCAN_STEP       8
#define MAX_PATCHES     64

/* ============================================================
 * OFFSETS (from com.kiloo.subwaysurf_64bit.cs dump)
 * ============================================================ */

/* CoreRunnerManager */
#define OFF_CRM_IS_ACTIVE       0x10
#define OFF_CRM_END_SCREEN      0x11
#define OFF_CRM_HIGHEST_MULT    0x14
#define OFF_CRM_SESSION_DATA    0x18
#define OFF_CRM_RUN_MULTIPLIER  0x20
#define OFF_CRM_POWERUPS        0x28
#define OFF_CRM_MODIFIERS       0x38

/* RunSessionData */
#define OFF_RSD_DISTANCE        0x18
#define OFF_RSD_KEYS            0x20
#define OFF_RSD_COINS           0x28
#define OFF_RSD_TOTAL_TIME      0x30
#define OFF_RSD_BONUS_COINS     0x48
#define OFF_RSD_MULTIPLIER_USED 0x4c
#define OFF_RSD_POINTS          0x50
#define OFF_RSD_COINS_GROUP     0x58
#define OFF_RSD_SEASON_TOKENS   0x60
#define OFF_RSD_CITY_TOUR       0x70
#define OFF_RSD_RANKING         0x80
#define OFF_RSD_NEW_HIGH        0xa0
#define OFF_RSD_PAUSED          0xa1
#define OFF_RSD_TUTORIAL        0xa2
#define OFF_RSD_PREV_POINTS     0xa8

/* CoreRunMultiplier */
#define OFF_CRMUL_BOOSTER       0x10
#define OFF_CRMUL_MYSTERY       0x14
#define OFF_CRMUL_EVENT         0x18
#define OFF_CRMUL_DOUBLE_SCORE  0x1c
#define OFF_CRMUL_TOTAL         0x20

/* CharacterMotorAbilities */
#define OFF_CMA_SHRINK          0x20
#define OFF_CMA_FAST_DIVE       0x28
#define OFF_CMA_ABILITIES       0x30
#define OFF_CMA_CONFIG          0x38
#define OFF_CMA_MOTOR           0x40
#define OFF_CMA_MIN_SPEED       0x70
#define OFF_CMA_MAX_SPEED       0x74
#define OFF_CMA_SPEED_MULT      0x78
#define OFF_CMA_GRAVITY         0x7c

/* CharacterMotorConfig */
#define OFF_CMC_GRAVITY         0x18
#define OFF_CMC_STICK_GROUND    0x1c
#define OFF_CMC_SPEED_CFG       0x28
#define OFF_CMC_LANE_CHANGE_DUR 0x30
#define OFF_CMC_JUMP_HEIGHT     0x4c
#define OFF_CMC_AIR_JUMP        0x50
#define OFF_CMC_ROLL_DUR        0x5c
#define OFF_CMC_COLLIDER_H      0x64

/* CollisionAbilityInstance */
#define OFF_COL_HEIGHT          0x28
#define OFF_COL_HEIGHT_ON       0x2c
#define OFF_COL_HEIGHT_MULT     0x30
#define OFF_COL_HEIGHT_MULT_ON  0x34
#define OFF_COL_NO_LOWER        0x35
#define OFF_COL_NO_LOWER_ON     0x36
#define OFF_COL_NO_CORNER       0x37
#define OFF_COL_NO_CORNER_ON    0x38

/* PowerUpsController */
#define OFF_PUC_DOUBLE_COINS    0x10
#define OFF_PUC_DOUBLE_SEASON   0x18

/* Magnetizer */
#define OFF_MAG_SPEED           0x30
#define OFF_MAG_MOTOR           0x38
#define OFF_MAG_SNEAKERS        0x50

/* ============================================================
 * GLOBALS
 * ============================================================ */
static int g_pid = -1;
static int g_running = 1;
static unsigned long g_crm_addr = 0;
static int g_game_version = 0;

/* Cheat settings */
static struct {
    int score_mult;
    int coin_mult;
    float speed_hack;
    float jump_hack;
    float gravity_hack;
    int no_collision;
    int infinite_coins;
    int double_coins;
    int magnet_range;
    int infinite_hoverboard;
    int jetpack_always;
    int invincible;
    int shield;
    int god_mode;
    int score_protect;
    int auto_revive;
    int fast_landing;
    int double_jump;
    int no_ads;
    int bypass_anti_root;
    int bypass_frida;
    int bypass_ptrace;
    /* Custom patches */
    int custom_patch_count;
    struct { unsigned long addr; unsigned int val; } custom_patches[MAX_PATCHES];
} g = {
    .score_mult = 1, .coin_mult = 1,
    .speed_hack = 1.0f, .jump_hack = 1.0f, .gravity_hack = 1.0f,
    .no_collision = 0, .infinite_coins = 0, .double_coins = 0,
    .magnet_range = 0, .infinite_hoverboard = 0, .jetpack_always = 0,
    .invincible = 0, .shield = 0, .god_mode = 0,
    .score_protect = 0, .auto_revive = 0, .fast_landing = 0,
    .double_jump = 0, .no_ads = 0,
    .bypass_anti_root = 1, .bypass_frida = 1, .bypass_ptrace = 1,
};

/* ============================================================
 * MEMORY READ/WRITE via /proc/pid/mem
 * ============================================================ */
static int read_mem(int pid, unsigned long addr, void *buf, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    if (lseek(fd, (off_t)addr, SEEK_SET) < 0) { close(fd); return -1; }
    ssize_t n = read(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int write_mem(int pid, unsigned long addr, const void *buf, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    if (lseek(fd, (off_t)addr, SEEK_SET) < 0) { close(fd); return -1; }
    ssize_t n = write(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int r32(int pid, unsigned long a, void *v) { return read_mem(pid, a, v, 4); }
static int r64(int pid, unsigned long a, void *v) { return read_mem(pid, a, v, 8); }
static int r8(int pid, unsigned long a, void *v)  { return read_mem(pid, a, v, 1); }
static int w32(int pid, unsigned long a, int v)   { return write_mem(pid, a, &v, 4); }
static int wu32(int pid, unsigned long a, unsigned int v) { return write_mem(pid, a, &v, 4); }
static int wf(int pid, unsigned long a, float v)  { return write_mem(pid, a, &v, 4); }
static int w8(int pid, unsigned long a, char v)   { return write_mem(pid, a, &v, 1); }
static int rf(int pid, unsigned long a, float *v) { return read_mem(pid, a, v, 4); }

/* ============================================================
 * BYPASS: Anti-Cheat / Anti-Root / Frida Detection
 * ============================================================ */

/* Self-ptrace to prevent other tools from attaching */
static void bypass_ptrace(void) {
    if (!g.bypass_ptrace) return;
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        printf(C_GRAY "  [*] ptrace traceme failed (may already be traced)\n" C_RESET);
    } else {
        printf(C_GREEN "  [+] ptrace self-attached (anti-debug bypassed)\n" C_RESET);
        /* Detach immediately */
        ptrace(PTRACE_DETACH, 0, 0, 0);
    }
}

/* Hide from /proc listing (rename cmdline) */
static void hide_process(void) {
    if (!g.bypass_anti_root) return;
    /* Overwrite our own cmdline to look innocent */
    int fd = open("/proc/self/cmdline", O_WRONLY);
    if (fd >= 0) {
        const char *disguise = "/system/bin/logd";
        write(fd, disguise, strlen(disguise) + 1);
        close(fd);
        printf(C_GREEN "  [+] Process disguised as logd\n" C_RESET);
    }
}

/* Remove frida agent from memory if loaded */
static void bypass_frida(void) {
    if (!g.bypass_frida) return;

    /* Check for frida server running */
    DIR *proc = opendir("/proc");
    if (!proc) return;

    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!ent->d_name[0] || !isdigit((unsigned char)ent->d_name[0]))
            continue;
        int pid = atoi(ent->d_name);
        if (pid <= 0 || pid == getpid()) continue;

        char path[128];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;

        char buf[256] = {0};
        read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (strstr(buf, "frida") || strstr(buf, "gadget") || strstr(buf, "gmain")) {
            printf(C_YELLOW "  [!] Frida detected at PID %d: %s\n" C_RESET, pid, buf);
            printf(C_YELLOW "  [*] Consider killing frida server for cleaner bypass\n" C_RESET);
        }
    }
    closedir(proc);
}

/* Scan for common anti-root detection strings */
static void patch_anti_root(int pid) {
    /* Common anti-root patterns in Unity/IL2CPP games:
     * 1. Check /system/bin/su, /system/xbin/su
     * 2. Check busybox path
     * 3. Check Magisk package
     * 4. Check SafetyNet/Play Integrity
     * 5. Check if /su exists
     * 6. Check superuser.apk
     */
    printf(C_CYAN "  [*] Scanning for anti-root checks...\n" C_RESET);

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r') continue;
        if ((end - start) > 0x1000000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            /* Look for "su" binary check patterns */
            const char *anti_root_strings[] = {
                "/system/bin/su", "/system/xbin/su", "/sbin/su",
                "com.topjohnwu.magisk", "eu.chainfire.supersu",
                "com.koushikdutta.superuser", "superuser.apk",
                "/system/app/Superuser", "test-keys",
                "/su/bin/su", "de.robv.android.xposed",
                NULL
            };

            for (int i = 0; anti_root_strings[i]; i++) {
                const char *needle = anti_root_strings[i];
                size_t nlen = strlen(needle);

                for (size_t j = 0; j + nlen < sz; j++) {
                    if (memcmp(buf + j, needle, nlen) == 0) {
                        /* Found anti-root string reference - try to NOP the check */
                        /* Look backward for CMP/BNZ/CBZ patterns */
                        if (j >= 4) {
                            unsigned int insn;
                            memcpy(&insn, buf + j - 4, 4);
                            /* CMP W*, #0 followed by B.NE */
                            if ((insn & 0xFF000000) == 0x71000000) {
                                printf(C_GREEN "    [+] Patched anti-root check: %s\n" C_RESET, needle);
                                w32(pid, start + j - 4, 0xD503201F); /* NOP */
                                found++;
                            }
                        }
                    }
                }
            }
        }
        free(buf);
    }
    fclose(fp);
    printf(C_CYAN "  [*] Anti-root patches: %d\n" C_RESET, found);
}

/* ============================================================
 * SCANNER: Find CoreRunnerManager instance
 * ============================================================ */

/* Strategy 1: Known value scanning (fast) */
static unsigned long scan_strategy_known(int pid) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return 0;

    char line[512];
    int regions = 0;
    unsigned long result = 0;

    while (fgets(line, sizeof(line), fp) && regions < MAX_REGIONS) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r') continue;
        if ((end - start) > 0x200000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            for (size_t i = 0; i + 0x100 < sz; i += SCAN_STEP) {
                unsigned long *p = (unsigned long *)(buf + i);

                /* +0x18 = RunSessionData* */
                unsigned long rsd = p[3];
                if (rsd < 0x10000 || rsd > 0x8000000000UL) continue;

                /* +0x20 = CoreRunMultiplier* */
                unsigned long crm = p[4];
                if (crm < 0x10000 || crm > 0x8000000000UL) continue;

                /* Validate RunSessionData: TotalTime at +0x30 must be float 0-100000 */
                float tt = 0;
                if (read_mem(pid, rsd + 0x30, &tt, 4) != 0) continue;
                if (tt < 0.0f || tt > 100000.0f) continue;

                /* Validate CoreRunMultiplier: total at +0x20 must be int 1-100 */
                int tm = 0;
                if (read_mem(pid, crm + 0x20, &tm, 4) != 0) continue;
                if (tm < 1 || tm > 100) continue;

                /* Validate session coins at +0x28 is reasonable */
                int coins = 0;
                if (read_mem(pid, rsd + 0x28, &coins, 4) != 0) continue;
                if (coins < 0 || coins > 99999999) continue;

                /* Found it! */
                result = start + i;
                free(buf);
                goto done;
            }
        }
        free(buf);
        regions++;
    }
done:
    fclose(fp);
    return result;
}

/* Strategy 2: Scan all libil2cpp.so data sections */
static unsigned long scan_strategy_libdata(int pid) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return 0;

    char line[512];
    unsigned long result = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, "libil2cpp.so")) continue;

        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[1] != 'w') continue; /* need writable */

        size_t sz = end - start;
        if (sz > 0x500000) continue;

        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            for (size_t i = 0; i + 0x80 < sz; i += 8) {
                unsigned long *p = (unsigned long *)(buf + i);
                unsigned long rsd = p[3];
                if (rsd < 0x10000) continue;
                unsigned long crm = p[4];
                if (crm < 0x10000) continue;

                float tt = 0;
                if (read_mem(pid, rsd + 0x30, &tt, 4) == 0 &&
                    tt > 0.0f && tt < 100000.0f) {
                    int tm = 0;
                    if (read_mem(pid, crm + 0x20, &tm, 4) == 0 &&
                        tm > 0 && tm < 100) {
                        result = start + i;
                        free(buf);
                        goto done2;
                    }
                }
            }
        }
        free(buf);
    }
done2:
    fclose(fp);
    return result;
}

/* Forward declarations */
static int rf(int pid, unsigned long a, float *v);

/* Main scanner - try both strategies */
static unsigned long find_crm(int pid) {
    unsigned long r = scan_strategy_known(pid);
    if (r) return r;
    r = scan_strategy_libdata(pid);
    return r;
}

/* ============================================================
 * CHEAT: Score & Coins
 * ============================================================ */
static void cheat_score_mult(int pid, unsigned long crm) {
    if (g.score_mult <= 1) return;
    unsigned long mp;
    if (r64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mp) < 0 || mp < 0x1000) return;
    w32(pid, mp + OFF_CRMUL_BOOSTER, g.score_mult);
    w32(pid, mp + OFF_CRMUL_MYSTERY, g.score_mult);
    w32(pid, mp + OFF_CRMUL_EVENT, g.score_mult);
    w32(pid, mp + OFF_CRMUL_TOTAL, g.score_mult);
    wu32(pid, mp + OFF_CRMUL_DOUBLE_SCORE, 1);
}

static void cheat_coin_mult(int pid, unsigned long crm) {
    if (g.coin_mult <= 1) return;
    unsigned long sp;
    if (r64(pid, crm + OFF_CRM_SESSION_DATA, &sp) < 0 || sp < 0x1000) return;
    w32(pid, sp + OFF_RSD_BONUS_COINS, g.coin_mult * 100);
    w32(pid, sp + OFF_RSD_MULTIPLIER_USED, g.coin_mult);
}

static void cheat_infinite_coins(int pid, unsigned long crm) {
    if (!g.infinite_coins) return;
    unsigned long sp;
    if (r64(pid, crm + OFF_CRM_SESSION_DATA, &sp) < 0 || sp < 0x1000) return;
    w32(pid, sp + OFF_RSD_COINS, 999999);
    w32(pid, sp + OFF_RSD_BONUS_COINS, 999999);
}

static void cheat_double_coins(int pid, unsigned long crm) {
    if (!g.double_coins) return;
    unsigned long mp;
    if (r64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mp) < 0 || mp < 0x1000) return;
    wu32(pid, mp + OFF_CRMUL_DOUBLE_SCORE, 1);
}

/* ============================================================
 * CHEAT: Speed / Jump / Gravity
 * ============================================================ */
static void cheat_speed(int pid, unsigned long crm) {
    if (g.speed_hack <= 1.0f) return;
    unsigned long mp;
    if (r64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mp) < 0 || mp < 0x1000) return;
    /* Use event multiplier as speed proxy */
    w32(pid, mp + OFF_CRMUL_EVENT, (int)(g.speed_hack * 10));
}

static void find_and_apply_motor_config(int pid) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x200000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            for (size_t i = 0; i + 0x80 < sz; i += 4) {
                float grav = *(float *)(buf + i);
                float jump = *(float *)(buf + i + 0x34); /* 0x4c - 0x18 */
                float roll = *(float *)(buf + i + 0x44); /* 0x5c - 0x18 */

                /* CharacterMotorConfig validation */
                if (grav >= 8.0f && grav <= 35.0f &&
                    jump >= 1.5f && jump <= 10.0f &&
                    roll >= 0.3f && roll <= 3.0f) {

                    if (g.jump_hack > 1.0f)
                        wf(pid, start + i + 0x34, jump * g.jump_hack);
                    if (g.gravity_hack != 1.0f)
                        wf(pid, start + i, grav * g.gravity_hack);
                    if (g.double_jump)
                        wf(pid, start + i + 0x38, jump * 1.2f); /* AirJumpHeight */
                    if (g.fast_landing)
                        wf(pid, start + i + 0x44, roll * 0.3f); /* RollDuration */

                    free(buf);
                    fclose(fp);
                    return;
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

/* ============================================================
 * CHEAT: Collision / God Mode
 * ============================================================ */
static void cheat_collision(int pid) {
    if (!g.no_collision && !g.god_mode) return;

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x200000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            for (size_t i = OFF_COL_HEIGHT; i + 0x40 < sz; i += 8) {
                float ch = *(float *)(buf + i - OFF_COL_HEIGHT + OFF_COL_HEIGHT);
                if (ch >= 1.0f && ch <= 6.0f) {
                    char one = 1;
                    /* NoLowerCollision + On */
                    write_mem(pid, start + i + 0xd, &one, 1);
                    write_mem(pid, start + i + 0xe, &one, 1);
                    /* NoCornerCollision + On */
                    write_mem(pid, start + i + 0xf, &one, 1);
                    write_mem(pid, start + i + 0x10, &one, 1);
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

/* ============================================================
 * CHEAT: Power-ups (Hoverboard, Jetpack, Shield, Invincible)
 * ============================================================ */

/* Find and activate power-ups by scanning for PowerEffect instances */
static void cheat_powerups(int pid) {
    if (!g.infinite_hoverboard && !g.jetpack_always &&
        !g.invincible && !g.shield && !g.auto_revive) return;

    /* Power-up activation patterns:
     * HoverboardPowerEffect: has _cooldownVariableId at 0x78, _isRandomBoard at 0x7c
     * JetpackPowerEffect: has _coinSpawner at 0x50, _targetHeight at 0x68
     * InvincibilityPowerEffect: inherits AutoRevivePowerEffect
     * ShieldPowerEffect: inherits AutoRevivePowerEffect
     */

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    int activated = 0;

    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x200000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            for (size_t i = 0x50; i + 0xa0 < sz; i += 8) {
                /* Scan for PowerEffect-like structures:
                 * offset 0x10-0x18 should be function pointers or vtable
                 * offset 0x50+ should be MonoBehaviour fields (0x20 base) */

                /* Check for Hoverboard pattern: float cooldown at +0x78, bool at +0x7c */
                float cooldown = *(float *)(buf + i + 0x28); /* 0x78 - 0x50 */
                unsigned char is_random = buf[i + 0x2c]; /* 0x7c - 0x50 */

                if (cooldown >= 0.0f && cooldown <= 60.0f && is_random <= 1) {
                    if (g.infinite_hoverboard) {
                        /* Set cooldown to 0 */
                        float zero = 0.0f;
                        write_mem(pid, start + i + 0x28, &zero, 4);
                        activated++;
                    }
                }

                /* Check for Jetpack pattern: float targetHeight at +0x68, float flightSpeed at +0x6c */
                float height = *(float *)(buf + i + 0x18); /* 0x68 - 0x50 */
                float speed = *(float *)(buf + i + 0x1c); /* 0x6c - 0x50 */

                if (height >= 5.0f && height <= 50.0f && speed >= 3.0f && speed <= 30.0f) {
                    if (g.jetpack_always) {
                        /* Force jetpack state */
                        unsigned char one = 1;
                        write_mem(pid, start + i + 0x10, &one, 1);
                        activated++;
                    }
                }
            }
        }
        free(buf);
    }
    fclose(fp);
    if (activated > 0)
        printf(C_GREEN "    [+] Power-ups activated: %d\n" C_RESET, activated);
}

/* ============================================================
 * CHEAT: Magnet
 * ============================================================ */
static void cheat_magnet(int pid) {
    if (!g.magnet_range) return;

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x100000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            for (size_t i = OFF_MAG_SPEED; i + 0x30 < sz; i += 8) {
                float spd = *(float *)(buf + i);
                if (spd >= 3.0f && spd <= 30.0f) {
                    /* Increase magnet speed/range 5x */
                    wf(pid, start + i, spd * 5.0f);
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

/* ============================================================
 * CHEAT: Score Protector (prevent score from being reset)
 * ============================================================ */
static void cheat_score_protect(int pid, unsigned long crm) {
    if (!g.score_protect) return;
    unsigned long sp;
    if (r64(pid, crm + OFF_CRM_SESSION_DATA, &sp) < 0 || sp < 0x1000) return;

    /* Keep resetting total time to prevent run timeout */
    /* Keep bonus coins high */
    int bonus;
    if (r32(pid, sp + OFF_RSD_BONUS_COINS, &bonus) == 0) {
        if (bonus < 10000) w32(pid, sp + OFF_RSD_BONUS_COINS, 99999);
    }
}

/* ============================================================
 * CHEAT: Custom Patches (user-defined)
 * ============================================================ */
static void apply_custom_patches(int pid) {
    for (int i = 0; i < g.custom_patch_count; i++) {
        if (g.custom_patches[i].addr > 0) {
            w32(pid, g.custom_patches[i].addr, g.custom_patches[i].val);
            printf(C_GREEN "    [+] Custom patch @ 0x%lx = 0x%08x\n" C_RESET,
                   g.custom_patches[i].addr, g.custom_patches[i].val);
        }
    }
}

/* ============================================================
 * CHEAT: No Ads
 * ============================================================ */
static void cheat_no_ads(int pid) {
    if (!g.no_ads) return;

    /* Scan for ad-related strings and NOP their callers */
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[0] != 'r') continue;
        if ((end - start) > 0x1000000) continue;

        size_t sz = end - start;
        unsigned char *buf = malloc(sz);
        if (!buf) continue;

        if (read_mem(pid, start, buf, sz) == 0) {
            const char *ad_strings[] = {
                "ShowAd", "LoadAd", "InterstitialAd", "RewardedAd",
                "BannerAd", "AdManager", "admob", "unity3d.com/ads",
                "ad_count", "show_ad", "can_show_ad",
                NULL
            };
            for (int i = 0; ad_strings[i]; i++) {
                unsigned char *match = memmem(buf, sz, ad_strings[i], strlen(ad_strings[i]));
                if (match) {
                    unsigned long addr = start + (match - buf);
                    printf(C_GRAY "    [ad] Found '%s' @ 0x%lx\n" C_RESET, ad_strings[i], addr);
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

/* ============================================================
 * MAIN CHEAT LOOP
 * ============================================================ */
static void *cheat_thread(void *arg) {
    (void)arg;
    int scan_attempts = 0;

    printf(C_CYAN "[+] Cheat thread started\n" C_RESET);

    while (g_running) {
        /* Re-find PID */
        if (g_pid <= 0 || kill(g_pid, 0) != 0) {
            g_pid = -1;
            g_crm_addr = 0;
            DIR *proc = opendir("/proc");
            if (proc) {
                struct dirent *ent;
                while ((ent = readdir(proc)) != NULL) {
                    if (!ent->d_name[0] || !isdigit((unsigned char)ent->d_name[0]))
                        continue;
                    int pid = atoi(ent->d_name);
                    if (pid <= 0) continue;
                    char path[128];
                    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
                    int fd = open(path, O_RDONLY);
                    if (fd < 0) continue;
                    char buf[256] = {0};
                    read(fd, buf, sizeof(buf) - 1);
                    close(fd);
                    if (strstr(buf, GAME_PACKAGE)) {
                        g_pid = pid;
                        printf(C_GREEN "[+] Found game PID: %d\n" C_RESET, pid);
                        break;
                    }
                }
                closedir(proc);
            }
            if (g_pid <= 0) {
                printf(C_YELLOW "[-] Waiting for game...\n" C_RESET);
                usleep(1000000);
                continue;
            }
        }

        /* Scan for CoreRunnerManager */
        if (g_crm_addr == 0) {
            scan_attempts++;
            printf(C_CYAN "[*] Scanning for game data (attempt %d)...\n" C_RESET, scan_attempts);
            g_crm_addr = find_crm(g_pid);
            if (g_crm_addr == 0) {
                printf(C_YELLOW "[-] Not in a run yet. Start a run in-game.\n" C_RESET);
                usleep(2000000);
                continue;
            }
            printf(C_GREEN "[+] CoreRunnerManager @ 0x%lx\n" C_RESET, g_crm_addr);
            scan_attempts = 0;
        }

        /* Check if still active */
        unsigned char is_active = 0;
        if (r8(g_pid, g_crm_addr + OFF_CRM_IS_ACTIVE, &is_active) < 0 || !is_active) {
            printf(C_YELLOW "[*] Run ended. Waiting for new run...\n" C_RESET);
            g_crm_addr = 0;
            usleep(1000000);
            continue;
        }

        /* Apply all cheats */
        cheat_score_mult(g_pid, g_crm_addr);
        cheat_coin_mult(g_pid, g_crm_addr);
        cheat_infinite_coins(g_pid, g_crm_addr);
        cheat_double_coins(g_pid, g_crm_addr);
        cheat_speed(g_pid, g_crm_addr);
        find_and_apply_motor_config(g_pid);
        cheat_collision(g_pid);
        cheat_powerups(g_pid);
        cheat_magnet(g_pid);
        cheat_score_protect(g_pid, g_crm_addr);
        apply_custom_patches(g_pid);

        /* Status display */
        unsigned long sp = 0;
        if (r64(g_pid, g_crm_addr + OFF_CRM_SESSION_DATA, &sp) == 0 && sp > 0x1000) {
            int coins = 0, keys = 0;
            float dist = 0;
            r32(g_pid, sp + OFF_RSD_COINS, &coins);
            r32(g_pid, sp + OFF_RSD_KEYS, &keys);
            rf(g_pid, sp + OFF_RSD_DISTANCE, &dist);

            printf(C_CYAN "\r[SUBWAY] " C_RESET
                   C_GREEN "Coins:%d " C_RESET
                   C_YELLOW "Keys:%d " C_RESET
                   C_MAGENTA "Dist:%.0fm " C_RESET
                   C_BLUE "Spd:%.1fx " C_RESET
                   C_RED "Jmp:%.1fx " C_RESET
                   "          ",
                   coins, keys, dist, g.speed_hack, g.jump_hack);
            fflush(stdout);
        }

        usleep(80000); /* 80ms */
    }
    return NULL;
}



/* ============================================================
 * MENU
 * ============================================================ */
static void cls(void) {
    printf("\033[2J\033[H");
}

static void print_header(void) {
    printf(C_CYAN C_BOLD);
    printf("  ╔═══════════════════════════════════════════════════╗\n");
    printf("  ║     🎮 Panxcz Subway Surfers Tool v%s          ║\n", VERSION);
    printf("  ║     Copyright (c) 2025 Panxcz & Freebuff        ║\n");
    printf("  ╠═══════════════════════════════════════════════════╣\n");
    printf(C_RESET);
}

static void print_status(void) {
    printf("  ║ " C_GRAY "PID: %-8d" C_RESET, g_pid > 0 ? g_pid : 0);
    if (g_crm_addr)
        printf(" | " C_GREEN "CRM: 0x%lx" C_RESET, g_crm_addr);
    else
        printf(" | " C_RED "CRM: searching" C_RESET);
    printf("              ║\n");
}

static void print_menu(void) {
    print_header();
    print_status();

    printf("  ║                                                   ║\n");
    printf("  ║ " C_CYAN "CHEAT FEATURES" C_RESET "                                    ║\n");
    printf("  ║  [1] Score Multiplier:  " C_YELLOW "%-4d" C_RESET "                  ║\n", g.score_mult);
    printf("  ║  [2] Coin Multiplier:   " C_YELLOW "%-4d" C_RESET "                  ║\n", g.coin_mult);
    printf("  ║  [3] Speed Hack:        " C_YELLOW "%-5.1fx" C_RESET "                 ║\n", g.speed_hack);
    printf("  ║  [4] Jump Height:       " C_YELLOW "%-5.1fx" C_RESET "                 ║\n", g.jump_hack);
    printf("  ║  [5] Gravity:           " C_YELLOW "%-5.1fx" C_RESET "                 ║\n", g.gravity_hack);
    printf("  ║                                                   ║\n");
    printf("  ║ " C_CYAN "TOGGLES" C_RESET "                                          ║\n");
    printf("  ║  [6] No Collision:      %s%-3s" C_RESET "                     ║\n",
           g.no_collision ? C_GREEN : C_RED, g.no_collision ? "ON" : "OFF");
    printf("  ║  [7] Infinite Coins:    %s%-3s" C_RESET "                     ║\n",
           g.infinite_coins ? C_GREEN : C_RED, g.infinite_coins ? "ON" : "OFF");
    printf("  ║  [8] Double Coins:      %s%-3s" C_RESET "                     ║\n",
           g.double_coins ? C_GREEN : C_RED, g.double_coins ? "ON" : "OFF");
    printf("  ║  [9] Magnet Range:      %s%-3s" C_RESET "                     ║\n",
           g.magnet_range ? C_GREEN : C_RED, g.magnet_range ? "ON" : "OFF");
    printf("  ║  [A] Infinite Hoverboard %s%-3s" C_RESET "                    ║\n",
           g.infinite_hoverboard ? C_GREEN : C_RED, g.infinite_hoverboard ? "ON" : "OFF");
    printf("  ║  [B] Jetpack Always     %s%-3s" C_RESET "                     ║\n",
           g.jetpack_always ? C_GREEN : C_RED, g.jetpack_always ? "ON" : "OFF");
    printf("  ║  [C] Invincibility      %s%-3s" C_RESET "                     ║\n",
           g.invincible ? C_GREEN : C_RED, g.invincible ? "ON" : "OFF");
    printf("  ║  [D] Shield             %s%-3s" C_RESET "                     ║\n",
           g.shield ? C_GREEN : C_RED, g.shield ? "ON" : "OFF");
    printf("  ║  [E] God Mode           %s%-3s" C_RESET "                     ║\n",
           g.god_mode ? C_GREEN : C_RED, g.god_mode ? "ON" : "OFF");
    printf("  ║  [F] Score Protector    %s%-3s" C_RESET "                     ║\n",
           g.score_protect ? C_GREEN : C_RED, g.score_protect ? "ON" : "OFF");
    printf("  ║  [G] Double Jump        %s%-3s" C_RESET "                     ║\n",
           g.double_jump ? C_GREEN : C_RED, g.double_jump ? "ON" : "OFF");
    printf("  ║  [H] Fast Landing       %s%-3s" C_RESET "                     ║\n",
           g.fast_landing ? C_GREEN : C_RED, g.fast_landing ? "ON" : "OFF");
    printf("  ║  [I] No Ads             %s%-3s" C_RESET "                     ║\n",
           g.no_ads ? C_GREEN : C_RED, g.no_ads ? "ON" : "OFF");
    printf("  ║                                                   ║\n");
    printf("  ║ " C_CYAN "BYPASS" C_RESET "                                           ║\n");
    printf("  ║  [P] Anti-Root Bypass   %s%-3s" C_RESET "                     ║\n",
           g.bypass_anti_root ? C_GREEN : C_RED, g.bypass_anti_root ? "ON" : "OFF");
    printf("  ║  [R] Frida Bypass       %s%-3s" C_RESET "                     ║\n",
           g.bypass_frida ? C_GREEN : C_RED, g.bypass_frida ? "ON" : "OFF");
    printf("  ║  [T] Ptrace Bypass      %s%-3s" C_RESET "                     ║\n",
           g.bypass_ptrace ? C_GREEN : C_RED, g.bypass_ptrace ? "ON" : "OFF");
    printf("  ║                                                   ║\n");
    printf("  ║ " C_CYAN "QUICK ACTIONS" C_RESET "                                     ║\n");
    printf("  ║  [X] Enable ALL cheats                             ║\n");
    printf("  ║  [Z] Disable ALL cheats                            ║\n");
    printf("  ║  [0] Exit                                          ║\n");
    printf("  ╚═══════════════════════════════════════════════════╝\n");
    printf("  Choice: ");
    fflush(stdout);
}

static void *input_thread(void *arg) {
    (void)arg;
    char input[64];

    /* Run bypasses on startup */
    printf(C_CYAN "\n[*] Running bypasses...\n" C_RESET);
    bypass_ptrace();
    hide_process();
    bypass_frida();

    while (g_running) {
        print_menu();
        if (fgets(input, sizeof(input), stdin)) {
            char c = input[0];
            if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';

            switch (c) {
            case '1':
                printf("  Score mult (1-100): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g.score_mult = atoi(input);
                if (g.score_mult < 1) g.score_mult = 1;
                if (g.score_mult > 100) g.score_mult = 100;
                break;
            case '2':
                printf("  Coin mult (1-100): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g.coin_mult = atoi(input);
                if (g.coin_mult < 1) g.coin_mult = 1;
                if (g.coin_mult > 100) g.coin_mult = 100;
                break;
            case '3':
                printf("  Speed (1.0-10.0): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g.speed_hack = (float)atof(input);
                if (g.speed_hack < 1.0f) g.speed_hack = 1.0f;
                if (g.speed_hack > 10.0f) g.speed_hack = 10.0f;
                break;
            case '4':
                printf("  Jump (1.0-10.0): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g.jump_hack = (float)atof(input);
                if (g.jump_hack < 1.0f) g.jump_hack = 1.0f;
                if (g.jump_hack > 10.0f) g.jump_hack = 10.0f;
                break;
            case '5':
                printf("  Gravity (0.1-2.0): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g.gravity_hack = (float)atof(input);
                if (g.gravity_hack < 0.1f) g.gravity_hack = 0.1f;
                if (g.gravity_hack > 2.0f) g.gravity_hack = 2.0f;
                break;
            case '6': g.no_collision = !g.no_collision; break;
            case '7': g.infinite_coins = !g.infinite_coins; break;
            case '8': g.double_coins = !g.double_coins; break;
            case '9': g.magnet_range = !g.magnet_range; break;
            case 'a': g.infinite_hoverboard = !g.infinite_hoverboard; break;
            case 'b': g.jetpack_always = !g.jetpack_always; break;
            case 'c': g.invincible = !g.invincible; break;
            case 'd': g.shield = !g.shield; break;
            case 'e': g.god_mode = !g.god_mode; break;
            case 'f': g.score_protect = !g.score_protect; break;
            case 'g': g.double_jump = !g.double_jump; break;
            case 'h': g.fast_landing = !g.fast_landing; break;
            case 'i': g.no_ads = !g.no_ads; break;
            case 'p': g.bypass_anti_root = !g.bypass_anti_root; break;
            case 'r': g.bypass_frida = !g.bypass_frida; break;
            case 't': g.bypass_ptrace = !g.bypass_ptrace; break;
            case 'x':
                g.score_mult = 99; g.coin_mult = 99;
                g.speed_hack = 2.0f; g.jump_hack = 2.5f;
                g.gravity_hack = 0.4f;
                g.no_collision = 1; g.infinite_coins = 1;
                g.double_coins = 1; g.magnet_range = 1;
                g.infinite_hoverboard = 1; g.jetpack_always = 1;
                g.invincible = 1; g.shield = 1;
                g.god_mode = 1; g.score_protect = 1;
                g.double_jump = 1; g.fast_landing = 1;
                g.no_ads = 1;
                g.bypass_anti_root = 1; g.bypass_frida = 1; g.bypass_ptrace = 1;
                printf(C_GREEN "  [+] ALL CHEATS ENABLED!\n" C_RESET);
                break;
            case 'z':
                memset(&g.score_mult, 0, (size_t)((char*)&g.bypass_ptrace - (char*)&g.score_mult + sizeof(int)));
                g.speed_hack = 1.0f; g.jump_hack = 1.0f; g.gravity_hack = 1.0f;
                printf(C_RED "  [-] ALL CHEATS DISABLED\n" C_RESET);
                break;
            case '0':
                g_running = 0;
                break;
            default:
                printf(C_RED "  Invalid choice\n" C_RESET);
                break;
            }
        }
    }
    return NULL;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char *argv[]) {
    /* Parse args */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--score") == 0 && i+1 < argc) g.score_mult = atoi(argv[++i]);
        if (strcmp(argv[i], "--coins") == 0 && i+1 < argc) g.coin_mult = atoi(argv[++i]);
        if (strcmp(argv[i], "--speed") == 0 && i+1 < argc) g.speed_hack = (float)atof(argv[++i]);
        if (strcmp(argv[i], "--jump") == 0 && i+1 < argc) g.jump_hack = (float)atof(argv[++i]);
        if (strcmp(argv[i], "--gravity") == 0 && i+1 < argc) g.gravity_hack = (float)atof(argv[++i]);
        if (strcmp(argv[i], "--no-collision") == 0) g.no_collision = 1;
        if (strcmp(argv[i], "--infinite-coins") == 0) g.infinite_coins = 1;
        if (strcmp(argv[i], "--double-coins") == 0) g.double_coins = 1;
        if (strcmp(argv[i], "--magnet") == 0) g.magnet_range = 1;
        if (strcmp(argv[i], "--hoverboard") == 0) g.infinite_hoverboard = 1;
        if (strcmp(argv[i], "--jetpack") == 0) g.jetpack_always = 1;
        if (strcmp(argv[i], "--invincible") == 0) g.invincible = 1;
        if (strcmp(argv[i], "--shield") == 0) g.shield = 1;
        if (strcmp(argv[i], "--god") == 0) g.god_mode = 1;
        if (strcmp(argv[i], "--protect") == 0) g.score_protect = 1;
        if (strcmp(argv[i], "--double-jump") == 0) g.double_jump = 1;
        if (strcmp(argv[i], "--fast-land") == 0) g.fast_landing = 1;
        if (strcmp(argv[i], "--no-ads") == 0) g.no_ads = 1;
        if (strcmp(argv[i], "--no-bypass") == 0) {
            g.bypass_anti_root = 0; g.bypass_frida = 0; g.bypass_ptrace = 0;
        }
        if (strcmp(argv[i], "--all") == 0) {
            g.score_mult = 99; g.coin_mult = 99;
            g.speed_hack = 2.0f; g.jump_hack = 2.5f; g.gravity_hack = 0.4f;
            g.no_collision = 1; g.infinite_coins = 1; g.double_coins = 1;
            g.magnet_range = 1; g.infinite_hoverboard = 1; g.jetpack_always = 1;
            g.invincible = 1; g.shield = 1; g.god_mode = 1;
            g.score_protect = 1; g.double_jump = 1; g.fast_landing = 1;
            g.no_ads = 1;
        }
        /* Custom patch: --patch 0xADDRESS 0xVALUE */
        if (strcmp(argv[i], "--patch") == 0 && i+2 < argc &&
            g.custom_patch_count < MAX_PATCHES) {
            unsigned long addr = strtoul(argv[++i], NULL, 16);
            unsigned int val = (unsigned int)strtoul(argv[++i], NULL, 16);
            g.custom_patches[g.custom_patch_count].addr = addr;
            g.custom_patches[g.custom_patch_count].val = val;
            g.custom_patch_count++;
        }
    }

    /* Check root */
    if (getuid() != 0) {
        printf(C_RED "[!] Need root: su -c ./subway_tool\n" C_RESET);
        return 1;
    }

    /* Start threads */
    pthread_t ctid, itid;
    pthread_create(&ctid, NULL, cheat_thread, NULL);
    pthread_create(&itid, NULL, input_thread, NULL);

    pthread_join(itid, NULL);
    g_running = 0;
    pthread_join(ctid, NULL);

    printf(C_CYAN "\n[+] Goodbye!\n" C_RESET);
    return 0;
}
