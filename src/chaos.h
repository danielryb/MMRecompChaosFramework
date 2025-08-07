#ifndef __CHAOS_H__
#define __CHAOS_H__

#include "modding.h"
#include "recomputils.h"
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

void chaos_init(void);
void chaos_update(GraphicsContext* gfxCtx, GameState* gameState);

extern bool chaos_is_player_active;

#endif /* __CHAOS_H__ */