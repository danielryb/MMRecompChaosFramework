#ifndef __CHAOS_H__
#define __CHAOS_H__

#include "util/debug.h"

#include "modding.h"
#include "recomputils.h"
#include "global.h"

#define GAME_CTX_ARG PlayState* play
#define GAME_CTX play

void chaos_init(void);
void chaos_update(GAME_CTX_ARG);

extern bool chaos_is_player_active;


#define FRAMES_PER_SECOND 20

#define INITIAL_MACHINE_COUNT 1

typedef void (*ChaosFunction)(GAME_CTX_ARG);

typedef enum {
    CHAOS_DISTURBANCE_VERY_LOW,
    CHAOS_DISTURBANCE_LOW,
    CHAOS_DISTURBANCE_MEDIUM,
    CHAOS_DISTURBANCE_HIGH,
    CHAOS_DISTURBANCE_VERY_HIGH,
    CHAOS_DISTURBANCE_NIGHTMARE,
    CHAOS_DISTURBANCE_MAX,
} ChaosDisturbance;


typedef struct {
    char* name;
    u32 duration; // In frames.

    ChaosFunction on_start_fun;
    ChaosFunction update_fun;
    ChaosFunction on_end_fun;
} ChaosEffect;


typedef enum {
    CHAOS_EFFECT_STATUS_AVAILABLE,
    CHAOS_EFFECT_STATUS_ACTIVE,
    CHAOS_EFFECT_STATUS_HIDDEN,
    CHAOS_EFFECT_STATUS_DISABLED,
} ChaosEffectStatus;

typedef struct {
    ChaosEffect effect;
    f32 weight_modifier;
    ChaosEffectStatus status;

    f32 left_available_weight_sum; // sum of weights of left subtree
} ChaosEffectEntity;


typedef struct {
    f32 initial_probability;
    f32 on_pick_multiplier;
    f32 winner_weight_share;
} ChaosGroupSettings;

typedef struct {
    ChaosGroupSettings settings;
    f32 probability;

    ChaosEffectEntity* effects;
    u32 effect_count;
    f32 shared_weight; // per effect
} ChaosGroup;


typedef struct {
    char* name;
    u32 cycle_length;
    ChaosGroupSettings default_groups_settings[CHAOS_DISTURBANCE_MAX];
} ChaosMachineSettings;

typedef struct e {
    ChaosEffectEntity* effect;
    ChaosGroup* group;
    u32 timer;
    struct e* next;
} ActiveChaosEffectList;

typedef struct {
    ChaosMachineSettings settings;
    ChaosGroup groups[CHAOS_DISTURBANCE_MAX];
    u32 cycle_timer;
    ActiveChaosEffectList* active_effects;
    ActiveChaosEffectList* remove_queue;
    u8 roll_requests;
    u8 group_roll_requests[CHAOS_DISTURBANCE_MAX];
} ChaosMachine;


typedef enum {
    CHAOS_STATE_DEFAULT,
    CHAOS_STATE_MACHINE_COUNT,
    CHAOS_STATE_MACHINE_REGISTER,
    CHAOS_STATE_EFFECT_COUNT,
    CHAOS_STATE_EFFECT_REGISTER,
    CHAOS_STATE_RUN,
} ChaosState;

extern ChaosMachine* machines;
extern u32 machine_count;
extern const char* DISTURBANCE_NAME[CHAOS_DISTURBANCE_MAX];

void active_list_add(ChaosMachine* machine, ChaosGroup* group, ChaosEffectEntity* entity, GAME_CTX_ARG);

#endif /* __CHAOS_H__ */
