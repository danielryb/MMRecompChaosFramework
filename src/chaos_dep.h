#ifndef __CHAOS_DEP_H__
#define __CHAOS_DEP_H__

#include "modding.h"
#include "global.h"

typedef struct ChaosEffectEntity ChaosEffectEntity;

typedef void (*ChaosFunction)(GraphicsContext* gfxCtx, GameState* gameState);
typedef u32 (*WeightModifier)(const ChaosEffectEntity* effect);

typedef struct {
    ChaosFunction fun;
    WeightModifier on_pick;
    WeightModifier on_skip;
    u32 weight;
} ChaosEffect;

struct ChaosEffectEntity {
    ChaosEffect effect;
    u32 current_weight;
};

RECOMP_IMPORT("mm_recomp_chaos_framework", void chaos_register_effect(ChaosEffect* effect))

#endif /* __CHAOS_DEP_H__ */