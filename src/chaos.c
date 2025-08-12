#include "chaos.h"

#define FRAMES_PER_SECOND 20

#define INITIAL_MACHINE_COUNT 1

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


typedef enum {
    CHAOS_EFFECT_STATUS_AVAILABLE,
    CHAOS_EFFECT_STATUS_ACTIVE,
    CHAOS_EFFECT_STATUS_HIDDEN,
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


typedef struct {
    char* name;
    u32 cycle_length;
    ChaosGroupSettings default_groups_settings[CHAOS_DISTURBANCE_MAX];
} ChaosMachineSettings;

typedef struct e {
    ChaosEffectEntity* effect;
    u32 timer;
    struct e* next;
} ActiveChaosEffectList;

typedef struct {
    ChaosMachineSettings settings;
    ChaosGroup groups[CHAOS_DISTURBANCE_MAX];
    u32 cycle_timer;
    ActiveChaosEffectList* active_effects;
} ChaosMachine;


typedef enum {
    CHAOS_STATE_DEFAULT,
    CHAOS_STATE_MACHINE_COUNT,
    CHAOS_STATE_MACHINE_REGISTER,
    CHAOS_STATE_EFFECT_COUNT,
    CHAOS_STATE_EFFECT_REGISTER,
    CHAOS_STATE_RUN,
} ChaosState;

ChaosState state;

ChaosMachine* machines;
u32 machine_count;


static const ChaosMachineSettings DEFAULT_MACHINE_SETTINGS = {
    .name = "*",
    .cycle_length = 15 * FRAMES_PER_SECOND,
    .default_groups_settings = {
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
        { /* CHAOS_DISTURBANCE_NIGHTMARE */
            .initial_probability = 0.0f,
            .on_pick_multiplier = 0.5f,
            .winner_weight_share = 1.0f,
        },
    }
};


RECOMP_DECLARE_EVENT(chaos_on_init(void));

RECOMP_EXPORT void chaos_enable_effect() {

}

RECOMP_EXPORT void chaos_disable_effect() {

}


RECOMP_EXPORT ChaosMachine* chaos_register_machine(const ChaosMachineSettings* settings) {
    switch (state) {
        case CHAOS_STATE_MACHINE_COUNT:
            machine_count++;
            break;
        case CHAOS_STATE_MACHINE_REGISTER:;
            u32 i = machine_count++;
            ChaosMachine* machine = &machines[i];

            machine->settings = *settings;
            machine->cycle_timer = 0;
            machine->active_effects = NULL;
            for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
                ChaosGroup* group = &machine->groups[j];
                group->settings = machine->settings.default_groups_settings[j];
                group->probability = group->settings.initial_probability;
            }

            return machine;
        case CHAOS_STATE_RUN:
        case CHAOS_STATE_DEFAULT:
            warning("Chaos machines can only be registered as callbacks to 'chaos_on_init'!");
            break;
        default:
            break;
    }
    return NULL;
}

RECOMP_EXPORT void chaos_register_effect_to(ChaosMachine* machine, const ChaosEffect* effect, ChaosDisturbance disturbance, const char** exclusivity_tags) {
    if (disturbance >= CHAOS_DISTURBANCE_MAX) {
        warning("Invalid disturbance provided!");
        return;
    }

    switch (state) {
        case CHAOS_STATE_EFFECT_COUNT:
            machine->groups[disturbance].effect_count++;
            break;
        case CHAOS_STATE_EFFECT_REGISTER:;
            u32 i = machine->groups[disturbance].effect_count++;
            ChaosEffectEntity* entity = &machine->groups[disturbance].effects[i];
            entity->effect = *effect;
            entity->current_weight = 1.0f;
            entity->status = CHAOS_EFFECT_STATUS_AVAILABLE;
            break;
        case CHAOS_STATE_RUN:
        case CHAOS_STATE_DEFAULT:
            warning("Chaos effects can only be registered as callbacks to 'chaos_on_init'!");
            break;
        default:
            break;
    }
}

RECOMP_EXPORT void chaos_register_effect(const ChaosEffect* effect, ChaosDisturbance disturbance, const char** exclusivity_tags) {
    chaos_register_effect_to(machines, effect, disturbance, exclusivity_tags);
}

static void reset_effect_counts(void) {
    for (u32 i = 0; i < machine_count; i++) {
        ChaosMachine* machine = &machines[i];
        for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
            ChaosGroup* group = &machine->groups[j];
            group->effect_count = 0;
        }
    }
}

void chaos_init(void) {
    state = CHAOS_STATE_MACHINE_COUNT;

    machine_count = 0;
    chaos_register_machine(&DEFAULT_MACHINE_SETTINGS);
    chaos_on_init();
    machines = recomp_alloc(sizeof(ChaosMachine) * machine_count);
    if (machines == NULL) {
        error("Couldn't allocate an array for chaos machines!");
        return;
    }

    state = CHAOS_STATE_MACHINE_REGISTER;

    machine_count = 0;
    chaos_register_machine(&DEFAULT_MACHINE_SETTINGS);
    chaos_on_init();

    for (u32 i = 0; i < machine_count; i++) {
        ChaosMachine* machine = &machines[i];
        for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
            ChaosGroup* group = &machine->groups[j];
            group->settings = machine->settings.default_groups_settings[j];
            group->probability = group->settings.initial_probability;
        }
    }

    state = CHAOS_STATE_EFFECT_COUNT;

    reset_effect_counts();
    chaos_on_init();

    for (u32 i = 0; i < machine_count; i++) {
        ChaosMachine* machine = &machines[i];
        for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
            ChaosGroup* group = &machine->groups[j];

            ChaosEffectEntity* new = recomp_alloc(sizeof(ChaosEffectEntity) * group->effect_count);
            if (new == NULL) {
                error("Couldn't allocate an array for chaos effects!");
                return;
            }
            group->effects = new;
        }
    }

    state = CHAOS_STATE_EFFECT_REGISTER;

    reset_effect_counts();
    chaos_on_init();

    state = CHAOS_STATE_RUN;
}

static void active_list_add(ChaosMachine* machine, ChaosEffectEntity* entity, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* new = recomp_alloc(sizeof(ActiveChaosEffectList));
    if (new == NULL) {
        error("Couldn't alloc an active list element!");
        return;
    }

    new->effect = entity;
    new->timer = 0;
    new->next = machine->active_effects;
    machine->active_effects = new;

    entity->status = CHAOS_EFFECT_STATUS_ACTIVE;

    ChaosEffect* effect = &entity->effect;
    if (effect->on_start_fun != NULL) {
        effect->on_start_fun(gfxCtx, gameState);
    }
}

static void active_list_remove_after(ChaosMachine* machine, ActiveChaosEffectList* element, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* del;

    if (element == NULL) {
        del = machine->active_effects;
        machine->active_effects = del->next;
    } else {
        del = element->next;
        element->next = del->next;
    }

    ChaosEffectEntity* entity = del->effect;
    ChaosEffect* effect = &entity->effect;

    entity->status = CHAOS_EFFECT_STATUS_AVAILABLE;

    if (effect->on_end_fun != NULL) {
        effect->on_end_fun(gfxCtx, gameState);
    }

    recomp_free(del);
}


static void active_list_update(ChaosMachine* machine, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* prev = NULL;

    for (ActiveChaosEffectList* cur = machine->active_effects; cur != NULL; cur = cur->next) {
        ChaosEffectEntity* entity = cur->effect;
        ChaosEffect* effect = &entity->effect;

        if (effect->update_fun != NULL) {
            effect->update_fun(gfxCtx, gameState);
        }

        if (cur->timer >= effect->duration) {
            active_list_remove_after(machine, prev, gfxCtx, gameState);
            continue;
        }
        cur->timer++;

        prev = cur;
    }
}

static ChaosGroup* pick_chaos_group(ChaosMachine* machine) {
    f32 rand = Rand_ZeroOne();

    for (int i = 0; i < CHAOS_DISTURBANCE_MAX; i++) {
        ChaosGroup* group = &machine->groups[i];

        if ((rand < group->probability) && (group->effect_count > 0)) {
            group->probability *= group->settings.on_pick_multiplier;
            return group;
        }
        rand -= group->probability;
    }

    return NULL;
}

static ChaosEffectEntity* pick_chaos_effect(ChaosGroup* group) {
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

static void machine_update(ChaosMachine* machine, GraphicsContext* gfxCtx, GameState* gameState) {
    u32 cycle_length = machine->settings.cycle_length;
    if (cycle_length > 0) {
        machine->cycle_timer++;
        if (machine->cycle_timer >= cycle_length) {
            ChaosGroup* group = pick_chaos_group(machine);

            if (group != NULL) {
                ChaosEffectEntity* effect = pick_chaos_effect(group);

                if (effect != NULL) {
                    active_list_add(machine, effect, gfxCtx, gameState);
                }
            }

            machine->cycle_timer = 0;
        }
    }

    active_list_update(machine, gfxCtx, gameState);
}

void chaos_update(GraphicsContext* gfxCtx, GameState* gameState) {
    for (u32 i = 0; i < machine_count; i++) {
        machine_update(&machines[i], gfxCtx, gameState);
    }
}