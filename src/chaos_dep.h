#ifndef __CHAOS_DEP_H__
#define __CHAOS_DEP_H__

#include "modding.h"
#include "global.h"

typedef void (*ChaosFunction)(GraphicsContext* gfxCtx, GameState* gameState);

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

typedef void ChaosEffectEntity;

typedef struct {
    f32 initial_probability;
    f32 on_pick_multiplier;
    f32 winner_weight_share;
} ChaosGroupSettings;

typedef struct {
    char* name;
    u32 cycle_length;
    ChaosGroupSettings default_groups_settings[CHAOS_DISTURBANCE_MAX];
} ChaosMachineSettings;

typedef void ChaosMachine;

RECOMP_IMPORT("mm_recomp_chaos_framework", ChaosEffectEntity* chaos_register_effect(ChaosEffect* effect, ChaosDisturbance disturbance, char** exclusivity_tags))
RECOMP_IMPORT("mm_recomp_chaos_framework", ChaosMachine* chaos_register_machine(ChaosMachineSettings* settings))
RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_enable_effect(ChaosEffectEntity* entity))
RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_disable_effect(ChaosEffectEntity* entity))
RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_request_roll(ChaosMachine* machine))

#endif /* __CHAOS_DEP_H__ */