/*
 * Subway Surfers Offsets - from com.kiloo.subwaysurf_64bit.cs
 * Package: com.kiloo.subwaysurf
 * Version: Latest (2025)
 * Architecture: ARM64 (aarch64)
 *
 * All offsets are FIELD offsets within their respective classes.
 * To find instance in memory: scan for known pointer patterns.
 */

#ifndef OFFSETS_H
#define OFFSETS_H

// ============================================================
// CoreRunnerManager - Main game state manager
// Found at: SYBO.Subway.CoreRunnerManager : IManager
// ============================================================
#define OFF_CRM_IS_IN_ACTIVE_RUN      0x10  // bool
#define OFF_CRM_HAS_SEEN_END_SCREEN   0x11  // bool
#define OFF_CRM_HIGHEST_MULT_EVER     0x14  // int
#define OFF_CRM_RUN_SESSION_DATA      0x18  // RunSessionData*
#define OFF_CRM_RUN_MULTIPLIER        0x20  // CoreRunMultiplier*
#define OFF_CRM_POWER_UPS_CTRL        0x28  // PowerUpsController*
#define OFF_CRM_ENFORCED_GAME_COUNT   0x30  // int
#define OFF_CRM_MODIFIERS             0x38  // ActiveGameModifiers*

// ============================================================
// RunSessionData - Score, coins, distance, keys
// ============================================================
#define OFF_RSD_DISTANCE              0x18  // SafeFloat _distance
#define OFF_RSD_KEYS                  0x20  // SafeInt _availableKeysForUse
#define OFF_RSD_COINS                 0x28  // SafeInt _availableCoinsForUse
#define OFF_RSD_TOTAL_TIME            0x30  // float TotalTime
#define OFF_RSD_BONUS_COINS           0x48  // int BonusCoins
#define OFF_RSD_MULTIPLIER_USED       0x4c  // int MultiplierUsed
#define OFF_RSD_POINTS                0x50  // RunVariableGroup Points (score)
#define OFF_RSD_COINS_GROUP           0x58  // RunVariableGroup Coins
#define OFF_RSD_SEASON_TOKENS         0x60  // RunVariableGroup SeasonTokens
#define OFF_RSD_CITY_TOUR_TOKENS      0x70  // RunVariableGroup CityTourTokens
#define OFF_RSD_RANKING_POINTS        0x80  // int RankingPoints
#define OFF_RSD_IS_NEW_HIGH_SCORE     0xa0  // bool IsNewHighScore

// ============================================================
// CoreRunMultiplier - Score multiplier system
// ============================================================
#define OFF_CRM_BOOSTER_MULT          0x10  // int _boosterMultiplier
#define OFF_CRM_MYSTERY_MULT          0x14  // int _mysteryPowerupMultiplier
#define OFF_CRM_EVENT_MULT            0x18  // int _eventScoreMultiplier
#define OFF_CRM_DOUBLE_SCORE_ACTIVE   0x1c  // bool _doubleScorePowerupActive
#define OFF_CRM_TOTAL_MULTIPLIER      0x20  // int _totalMultiplier

// ============================================================
// CharacterMotorAbilities - Speed, jump, gravity
// ============================================================
#define OFF_CMA_SHRINK_ABILITY        0x20  // CollisionAbility*
#define OFF_CMA_FAST_DIVE_ABILITY     0x28  // RollAbility*
#define OFF_CMA_ABILITIES_DICT        0x30  // Dictionary*
#define OFF_CMA_CONFIG                0x38  // CharacterMotorConfig*
#define OFF_CMA_MOTOR                 0x40  // CharacterMotor*
#define OFF_CMA_MOVEMENT_ABILITIES    0x48  // List*
#define OFF_CMA_JUMP_ABILITIES        0x50  // List*
#define OFF_CMA_LANE_CHANGE_ABILITIES 0x58  // List*
#define OFF_CMA_COLLISION_ABILITIES   0x60  // List*
#define OFF_CMA_ROLL_ABILITIES        0x68  // List*
#define OFF_CMA_MIN_SPEED             0x70  // float MinSpeed
#define OFF_CMA_MAX_SPEED             0x74  // float MaxSpeed
#define OFF_CMA_SPEED_MULTIPLIER      0x78  // float SpeedMultiplier
#define OFF_CMA_GRAVITY               0x7c  // float Gravity

// ============================================================
// CharacterMotorConfig - Physics config
// ============================================================
#define OFF_CMC_GRAVITY                0x18  // float Gravity
#define OFF_CMC_STICK_TO_GROUND       0x1c  // bool StickToGround
#define OFF_CMC_DEFAULT_SPEED_CFG     0x28  // CharacterSpeedConfig
#define OFF_CMC_LANE_CHANGE_DURATION  0x30  // float LaneChangeDuration
#define OFF_CMC_JUMP_HEIGHT           0x4c  // float JumpHeight
#define OFF_CMC_AIR_JUMP_HEIGHT       0x50  // float AirJumpHeight
#define OFF_CMC_ROLL_DURATION         0x5c  // float RollDuration
#define OFF_CMC_COLLIDER_HEIGHT       0x64  // float ColliderHeight

// ============================================================
// CharacterMotor - Runtime character state
// ============================================================
#define OFF_CM_POSITION               0x90  // Vector3 (approx)
#define OFF_CM_VELOCITY               0xa0  // Vector3 (approx)
#define OFF_CM_IS_GROUNDED            0xb0  // bool

// ============================================================
// CollisionAbilityInstance - Collision bypass
// ============================================================
#define OFF_COL_COLLIDER_HEIGHT       0x28  // float ColliderHeight
#define OFF_COL_COLLIDER_HEIGHT_ON    0x2c  // bool ColliderHeightOn
#define OFF_COL_COLLIDER_HEIGHT_MULT  0x30  // float ColliderHeightMultiplier
#define OFF_COL_NO_LOWER_COLLISION    0x35  // bool NoLowerCollision
#define OFF_COL_NO_LOWER_COLLISION_ON 0x36  // bool NoLowerCollisionOn
#define OFF_COL_NO_CORNER_COLLISION   0x37  // bool NoCornerCollision
#define OFF_COL_NO_CORNER_COLLISION_ON 0x38 // bool NoCornerCollisionOn

// ============================================================
// Magnetizer - Magnet range
// ============================================================
#define OFF_MAG_SPEED                 0x30  // float _speed
#define OFF_MAG_CHARACTER_MOTOR       0x38  // CharacterMotor*
#define OFF_MAG_IS_SNEAKERS_MAGNET    0x50  // bool _isSneakersMagnet

// ============================================================
// Power system
// ============================================================
#define OFF_POWER_TYPE                0x10  // PowerType enum
#define OFF_POWER_DURATION            0x14  // DurationType
#define OFF_POWER_ACTIVATION          0x18  // ActivationType
#define OFF_POWER_PAUSING             0x1c  // PowerFlags

// PowerTypes (enum values)
#define POWER_TYPE_HOVERBOARD         0x01
#define POWER_TYPE_JETPACK            0x02
#define POWER_TYPE_MAGNET             0x03
#define POWER_TYPE_POGO_STICK         0x04
#define POWER_TYPE_SCORE_MULTIPLIER   0x05
#define POWER_TYPE_DOUBLE_COINS       0x06
#define POWER_TYPE_INVINCIBILITY      0x07
#define POWER_TYPE_SHIELD             0x08
#define POWER_TYPE_TELEPORT           0x09

// ============================================================
// RunnerStateMachine - Game state
// ============================================================
#define OFF_RSM_CURRENT_STATE         0x20  // TStateType _currentState
#define OFF_RSM_STATE_MAP             0x30  // Dictionary*

// Game states
#define STATE_MAIN_MENU               0x00
#define STATE_ENTER_RUN               0x01
#define STATE_RUNNING                 0x02
#define STATE_END_RUN                 0x03
#define STATE_PAUSED                  0x04

// ============================================================
// UI Elements
// ============================================================
#define OFF_UI_MANAGER                0x00  // UIManager instance

// ============================================================
// Safe types (Unity encrypted primitives)
// ============================================================
// SafeFloat: XOR encrypted float, key stored nearby
// SafeInt: XOR encrypted int, key stored nearby
// To decrypt: value ^ key (key is usually at offset+4 or nearby)

#endif /* OFFSETS_H */
