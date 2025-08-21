#ifndef __CHAOS_DEP_H__
#define __CHAOS_DEP_H__

#include "modding.h"
#include "global.h"

typedef void (*ChaosFunction)(PlayState* play);

typedef struct {
    char* name;
    u32 duration; // In frames.

    ChaosFunction on_start_fun;
    ChaosFunction update_fun;
    ChaosFunction on_end_fun;
} ChaosEffect;

typedef void ChaosEffectEntity;

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
    f32 initial_probability;
    f32 on_pick_multiplier;
    f32 winner_weight_share;
} ChaosGroupSettings;

typedef struct {
    char* name;
    u32 cycle_length; // In frames.
    ChaosGroupSettings default_groups_settings[CHAOS_DISTURBANCE_MAX];
} ChaosMachineSettings;

typedef void ChaosMachine;

RECOMP_IMPORT("mm_recomp_chaos_framework", ChaosEffectEntity* chaos_register_effect_to(ChaosMachine* machine, const ChaosEffect* effect, ChaosDisturbance disturbance, const char** exclusivity_tags))
RECOMP_IMPORT("mm_recomp_chaos_framework", ChaosEffectEntity* chaos_register_effect(const ChaosEffect* effect, ChaosDisturbance disturbance, const char** exclusivity_tags))
RECOMP_IMPORT("mm_recomp_chaos_framework", ChaosMachine* chaos_register_machine(const ChaosMachineSettings* settings))

RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_enable_effect(ChaosEffectEntity* entity))
RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_disable_effect(ChaosEffectEntity* entity))
RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_stop_effect(ChaosEffectEntity* entity))

RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_request_roll(ChaosMachine* machine))
RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_request_group_roll(ChaosMachine* machine, ChaosDisturbance disturbance))

#endif /* __CHAOS_DEP_H__ */