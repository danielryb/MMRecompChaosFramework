#include "chaos.h"

#define FRAMES_PER_SECOND 20
#define CFG_CHAOS_CYCLE_LENGTH (30 * FRAMES_PER_SECOND) // In frames.

typedef enum {
    CHAOS_MACHINE_STATE_DEFAULT,
    CHAOS_MACHINE_STATE_COUNT,
    CHAOS_MACHINE_STATE_REGISTER,
    CHAOS_MACHINE_STATE_RUN,
} ChaosMachineState;

ChaosMachineState state = CHAOS_MACHINE_STATE_DEFAULT;

ChaosEffectEntity* effects;
u32 effect_count = 0;

u32 timer = 0;

RECOMP_DECLARE_EVENT(chaos_on_init(void));

RECOMP_EXPORT void chaos_register_effect(ChaosEffect* effect) {
    static u32 pos = 0;
    switch (state) {
        case CHAOS_MACHINE_STATE_COUNT:
            effect_count++;
            break;
        case CHAOS_MACHINE_STATE_REGISTER:;
            ChaosEffectEntity* entity = &effects[pos];
            entity->effect = (*effect);
            entity->current_weight = effect->weight;
            pos++;
            break;
        default:
            recomp_printf("WARNING Chaos effects can only be registered as callbacks to 'chaos_on_init'!");
            break;
    }
}

void chaos_init(void) {
    state = CHAOS_MACHINE_STATE_COUNT;

    chaos_on_init();
    effects = recomp_alloc(sizeof(ChaosEffectEntity) * effect_count);

    state = CHAOS_MACHINE_STATE_REGISTER;

    chaos_on_init();

    state = CHAOS_MACHINE_STATE_RUN;
}

ChaosFunction pick_chaos_function(void) {
    u32 weight_count = 0;
    for (u32 i = 0; i < effect_count; i++) {
        weight_count += effects[i].current_weight;
    }

    u32 rand;
    if (weight_count > 0) {
        rand = Rand_Next() % weight_count;
    } else {
        rand = 0;
    }

    u32 pos;
    for (pos = 0; pos < effect_count; pos++) {
        if (rand < effects[pos].current_weight) {
            break;
        }
        rand -= effects[pos].current_weight;
    }

    for (u32 i = 0; i < effect_count; i++) {
        if (i == pos) {
            continue;
        }

        ChaosEffectEntity* entity = &effects[i];
        entity->current_weight = entity->effect.on_skip(entity);
    }

    ChaosEffectEntity* pick = &effects[pos];
    pick->current_weight = pick->effect.on_pick(pick);

    return pick->effect.fun;
}

void chaos_update(GraphicsContext* gfxCtx, GameState* gameState) {
    if (timer >= CFG_CHAOS_CYCLE_LENGTH) {
        ChaosFunction fun = pick_chaos_function();
        fun(gfxCtx, gameState);

        timer = 0;
    } else {
        timer++;
    }
}