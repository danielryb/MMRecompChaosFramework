#include "chaos.h"

#define FRAMES_PER_SECOND 20
#define CFG_CHAOS_CYCLE_LENGTH (15 * FRAMES_PER_SECOND) // In frames.

typedef enum {
    CHAOS_MACHINE_STATE_DEFAULT,
    CHAOS_MACHINE_STATE_EFFECT_COUNT,
    CHAOS_MACHINE_STATE_REGISTER,
    CHAOS_MACHINE_STATE_RUN,
} ChaosMachineState;

ChaosMachineState state = CHAOS_MACHINE_STATE_DEFAULT;

typedef enum {
    CHAOS_EFFECT_STATUS_AVAILABLE,
    CHAOS_EFFECT_STATUS_ACTIVE,
    CHAOS_EFFECT_STATUS_DISABLED,
} ChaosEffectStatus;

typedef struct {
    ChaosEffect effect;
    f32 current_weight;
    ChaosEffectStatus status;
} ChaosEffectEntity;

typedef struct {
    f32 initial_probability;
    f32 on_pick_multiplier;
    f32 winner_weight_share;
} ChaosGroupSettings;

typedef struct {
    ChaosGroupSettings settings;
    ChaosEffectEntity* effects;
    u32 effect_count;
    f32 probability;
} ChaosGroup;

ChaosGroup chaos_groups[CHAOS_DISTURBANCE_COUNT];
u32 timer = 0;

typedef struct e {
    ChaosEffectEntity* effect;
    u32 timer;
    struct e* next;
} ActiveChaosEffectList;

ActiveChaosEffectList* active_effects;

static const ChaosGroupSettings DEFAULT_CHAOS_GROUPS_SETTINGS[CHAOS_DISTURBANCE_COUNT] = {
    { /* CHAOS_DISTURBANCE_VERY_LOW */
        .initial_probability = 0.3f,
        .on_pick_multiplier = 1.0f,
        .winner_weight_share = 0.2f,
    },
    { /* CHAOS_DISTURBANCE_LOW */
        .initial_probability = 0.2f,
        .on_pick_multiplier = 1.0f,
        .winner_weight_share = 0.5f,
    },
    { /* CHAOS_DISTURBANCE_MEDIUM */
        .initial_probability = 0.1f,
        .on_pick_multiplier = 1.0f,
        .winner_weight_share = 0.8f,
    },
    { /* CHAOS_DISTURBANCE_HIGH */
        .initial_probability = 0.05f,
        .on_pick_multiplier = 0.8f,
        .winner_weight_share = 1.0f,
    },
    { /* CHAOS_DISTURBANCE_VERY_HIGH */
        .initial_probability = 0.01f,
        .on_pick_multiplier = 0.8f,
        .winner_weight_share = 1.0f,
    },
};

RECOMP_DECLARE_EVENT(chaos_on_init(void));

RECOMP_EXPORT void chaos_register_effect(ChaosEffect* effect, ChaosDisturbance disturbance, char** exclusivity_tags) {
    static u32 pos[CHAOS_DISTURBANCE_COUNT];

    if (disturbance >= CHAOS_DISTURBANCE_COUNT) {
        recomp_printf("WARNING Invalid disturbance provided!");
        return;
    }

    switch (state) {
        case CHAOS_MACHINE_STATE_EFFECT_COUNT:
            chaos_groups[disturbance].effect_count++;
            break;
        case CHAOS_MACHINE_STATE_REGISTER:;
            u32 i = pos[disturbance]++;
            ChaosEffectEntity* entity = &chaos_groups[disturbance].effects[i];
            entity->effect = (*effect);
            entity->current_weight = 1.0f;
            entity->status = CHAOS_EFFECT_STATUS_AVAILABLE;
            break;
        default:
            recomp_printf("WARNING Chaos effects can only be registered as callbacks to 'chaos_on_init'!");
            break;
    }
}

void chaos_init(void) {
    for (int i = 0; i < CHAOS_DISTURBANCE_COUNT; i++) {
        ChaosGroup* group = &chaos_groups[i];
        group->settings = DEFAULT_CHAOS_GROUPS_SETTINGS[i];
        group->probability = group->settings.initial_probability;
    }

    state = CHAOS_MACHINE_STATE_EFFECT_COUNT;

    chaos_on_init();
    for (int i = 0; i < CHAOS_DISTURBANCE_COUNT; i++) {
        ChaosGroup* group = &chaos_groups[i];

        ChaosEffectEntity* new = recomp_alloc(sizeof(ChaosEffectEntity) * group->effect_count);
        if (new == NULL) {
            recomp_printf("ERROR Couldn't alloc an array for chaos effects!");
            return;
        }
        group->effects = new;
    }

    state = CHAOS_MACHINE_STATE_REGISTER;

    chaos_on_init();

    state = CHAOS_MACHINE_STATE_RUN;
}

void active_list_add(ChaosEffectEntity* effect, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* new = recomp_alloc(sizeof(ActiveChaosEffectList));
    if (new == NULL) {
        recomp_printf("ERROR Couldn't alloc an active list element!");
        return;
    }

    new->effect = effect;
    new->timer = 0;
    new->next = active_effects;
    active_effects = new;

    effect->status = CHAOS_EFFECT_STATUS_ACTIVE;
    effect->effect.on_start_fun(gfxCtx, gameState);
}

void active_list_remove_after(ActiveChaosEffectList* element, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* del;

    if (element == NULL) {
        del = active_effects;
        active_effects = del->next;
    } else {
        del = element->next;
        element->next = del->next;
    }

    ChaosEffectEntity* entity = del->effect;
    ChaosEffect* effect = &entity->effect;

    entity->status = CHAOS_EFFECT_STATUS_AVAILABLE;
    effect->on_end_fun(gfxCtx, gameState);

    recomp_free(del);
}


void active_list_update(GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* prev = NULL;

    for (ActiveChaosEffectList* cur = active_effects; cur != NULL; cur = cur->next) {
        ChaosEffectEntity* entity = cur->effect;
        ChaosEffect* effect = &entity->effect;

        effect->update_fun(gfxCtx, gameState);

        if (cur->timer >= effect->duration) {
            active_list_remove_after(prev, gfxCtx, gameState);
            continue;
        }
        cur->timer++;

        prev = cur;
    }
}

ChaosGroup* pick_chaos_group(void) {
    f32 rand = Rand_ZeroOne();

    for (int i = 0; i < CHAOS_DISTURBANCE_COUNT; i++) {
        ChaosGroup* group = &chaos_groups[i];

        if ((rand < group->probability) && (group->effect_count > 0)) {
            group->probability *= group->settings.on_pick_multiplier;
            return group;
        }
        rand -= group->probability;
    }

    return NULL;
}

ChaosEffectEntity* pick_chaos_effect(ChaosGroup* group) {
    u32 effect_count = group->effect_count;
    f32 rand = Rand_ZeroOne() * effect_count;

    u32 pos;
    for (pos = 0; pos < effect_count; pos++) {
        ChaosEffectEntity* entity = &group->effects[pos];
        if (rand < entity->current_weight) {
            break;
        }
        rand -= entity->current_weight;
    }

    ChaosEffectEntity* choice = &group->effects[pos];

    if (effect_count > 1) {
        f32 weight_share = choice->current_weight * group->settings.winner_weight_share;
        choice->current_weight -= weight_share;
        f32 share_per_effect = weight_share / (effect_count - 1);

        for (pos = 0; pos < effect_count; pos++) {
            ChaosEffectEntity* entity = &group->effects[pos];
            if (entity == choice) {
                continue;
            }

            entity->current_weight += share_per_effect;
        }
    }

    return choice;
}

void chaos_update(GraphicsContext* gfxCtx, GameState* gameState) {
    if (timer >= CFG_CHAOS_CYCLE_LENGTH) {
        ChaosGroup* group = pick_chaos_group();

        if (group != NULL) {
            ChaosEffectEntity* effect = pick_chaos_effect(group);

            if (effect != NULL) {
                active_list_add(effect, gfxCtx, gameState);
            }
        }

        timer = 0;
    } else {
        timer++;
    }

    active_list_update(gfxCtx, gameState);
}