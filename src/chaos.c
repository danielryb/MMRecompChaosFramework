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

static const char* DISTURBANCE_NAME[CHAOS_DISTURBANCE_MAX] = {
    "VERY_LOW",
    "LOW",
    "MEDIUM",
    "HIGH",
    "VERY_HIGH",
    "NIGHTMARE",
};

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

static inline ChaosDisturbance get_group_disturbance(ChaosMachine* machine, ChaosGroup* group) {
    return group - machine->groups;
}

static ChaosMachine* find_machine(void* ptr) {
    u32 l = 0;
    u32 r = machine_count;

    while (l != r) {
        u32 m = (r + l) / 2;
        void* m_ptr = &machines[m];

        if (ptr < m_ptr) {
            r = m;
        } else {
            l = m;
        }
    }

    return &machines[l];
}

static ChaosGroup* find_group(ChaosMachine* machine, void* ptr) {
    u32 l = 0;
    u32 r = CHAOS_DISTURBANCE_MAX;

    while (l != r) {
        u32 m = (r + l) / 2;
        void* m_ptr = &machine->groups[m];

        if (ptr < m_ptr) {
            r = m;
        } else {
            l = m;
        }
    }

    return &machine->groups[l];
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
            machine->remove_queue = NULL;
            machine->roll_requests = 0;
            for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
                ChaosGroup* group = &machine->groups[j];
                group->settings = machine->settings.default_groups_settings[j];
                group->probability = group->settings.initial_probability;
                group->shared_weight = 1.0f;
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

RECOMP_EXPORT ChaosEffectEntity* chaos_register_effect_to(ChaosMachine* machine, const ChaosEffect* effect, ChaosDisturbance disturbance, const char** exclusivity_tags) {
    if (disturbance >= CHAOS_DISTURBANCE_MAX) {
        warning("Invalid disturbance provided!");
        return NULL;
    }

    switch (state) {
        case CHAOS_STATE_EFFECT_COUNT:
            machine->groups[disturbance].effect_count++;
            break;
        case CHAOS_STATE_EFFECT_REGISTER:;
            u32 i = machine->groups[disturbance].effect_count++;
            ChaosEffectEntity* entity = &machine->groups[disturbance].effects[i];
            entity->effect = *effect;
            entity->weight_modifier = 0.0f;
            entity->status = CHAOS_EFFECT_STATUS_AVAILABLE;
            entity->left_available_weight_sum = 0.0f;

            return entity;
        case CHAOS_STATE_RUN:
        case CHAOS_STATE_DEFAULT:
            warning("Chaos effects can only be registered as callbacks to 'chaos_on_init'!");
            break;
        default:
            break;
    }
    return NULL;
}

RECOMP_EXPORT ChaosEffectEntity* chaos_register_effect(const ChaosEffect* effect, ChaosDisturbance disturbance, const char** exclusivity_tags) {
    return chaos_register_effect_to(machines, effect, disturbance, exclusivity_tags);
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

static f32 get_effect_entity_weight(ChaosGroup* group, ChaosEffectEntity* entity) {
    if (entity->status != CHAOS_EFFECT_STATUS_AVAILABLE) {
        return 0.0f;
    }
    return group->shared_weight + entity->weight_modifier;
}

static void generate_weight_tree(ChaosGroup* group) {
    u32 effect_count = group->effect_count;
    for (u32 i = effect_count; i > 0; i--) {
        u32 pos = i - 1;
        ChaosEffectEntity* entity = &group->effects[pos];

        u32 left_child_pos = pos * 2 + 1;
        if (left_child_pos >= effect_count) {
            continue;
        }

        ChaosEffectEntity* left_child = &group->effects[left_child_pos];
        entity->left_available_weight_sum = left_child->left_available_weight_sum + get_effect_entity_weight(group, left_child);
    }
}

static f32 get_available_weight_sum(ChaosGroup* group) {
    f32 sum = 0.0f;

    u32 pos = 0;
    while (pos < group->effect_count) {
        ChaosEffectEntity* entity = &group->effects[pos];
        sum += entity->left_available_weight_sum + get_effect_entity_weight(group, entity);

        pos = pos * 2 + 2;
    }

    return sum;
}

static ChaosEffectEntity* get_effect_entity_by_weight(ChaosGroup* group, f32 weight) {
    u32 pos = 0;
    while (pos < group->effect_count) {
        ChaosEffectEntity* entity = &group->effects[pos];
        f32 l = entity->left_available_weight_sum;
        f32 r = l + get_effect_entity_weight(group, entity);

        if (weight < l) {
            pos = pos * 2 + 1;
        } else if (weight >= r) {
            weight -= r;
            pos = pos * 2 + 2;
        } else {
            break;
        }
    }

    return &group->effects[pos];
}

static inline u32 get_effect_entity_pos(ChaosGroup* group, ChaosEffectEntity* entity) {
    u32 dist = entity - group->effects;
    return dist / sizeof(ChaosEffectEntity);
}

static void update_weight_sums_upwards(ChaosGroup* group, ChaosEffectEntity* entity) {
    u32 pos = get_effect_entity_pos(group, entity);
    while (pos > 0) {
        ChaosEffectEntity* left = &group->effects[pos];
        pos = (pos - 1) / 2;

        ChaosEffectEntity* entity = &group->effects[pos];
        entity->left_available_weight_sum = left->left_available_weight_sum + get_effect_entity_weight(group, left);
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


    debug_log("Detected %d chaos machine registration%s.", machine_count, ((machine_count != 1) ? "s" : ""));

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

        debug_log("Created '%s' chaos machine.", machine->settings.name);
    }

    state = CHAOS_STATE_EFFECT_COUNT;

    reset_effect_counts();
    chaos_on_init();

    for (u32 i = 0; i < machine_count; i++) {
        u32 total_count = 0;

        ChaosMachine* machine = &machines[i];
        for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
            ChaosGroup* group = &machine->groups[j];

            ChaosEffectEntity* new = recomp_alloc(sizeof(ChaosEffectEntity) * group->effect_count);
            if (new == NULL) {
                error("Couldn't allocate an array for chaos effects!");
                return;
            }
            group->effects = new;

            total_count += group->effect_count;
        }

        debug_log("Detected %d chaos effect registration%s to '%s'.", total_count, ((total_count != 1) ? "s" : ""), machine->settings.name);
    }

    state = CHAOS_STATE_EFFECT_REGISTER;

    reset_effect_counts();
    chaos_on_init();

    for (u32 i = 0; i < machine_count; i++) {
        ChaosMachine* machine = &machines[i];
        for (int j = 0; j < CHAOS_DISTURBANCE_MAX; j++) {
            ChaosGroup* group = &machine->groups[j];
            generate_weight_tree(group);

            for (u32 k = 0; k < group->effect_count; k++) {
                ChaosEffectEntity* entity = &group->effects[k];

                debug_log("Registered '%s' effect to '%s' chaos machine with %s disturbance.", entity->effect.name, machine->settings.name, DISTURBANCE_NAME[j]);
            }
        }
    }

    state = CHAOS_STATE_RUN;
}

static inline void effect_start(ChaosEffect* effect, GraphicsContext* gfxCtx, GameState* gameState) {
    if (effect->on_start_fun != NULL) {
        effect->on_start_fun(gfxCtx, gameState);
    }

    debug_log("Effect '%s' started.", effect->name);
}

static inline void effect_update(ChaosEffect* effect, GraphicsContext* gfxCtx, GameState* gameState) {
    if (effect->update_fun != NULL) {
        effect->update_fun(gfxCtx, gameState);
    }
}

static inline void effect_end(ChaosEffect* effect, GraphicsContext* gfxCtx, GameState* gameState) {
    if (effect->on_end_fun != NULL) {
        effect->on_end_fun(gfxCtx, gameState);
    }

    debug_log("Effect '%s' ended.", effect->name);
}

static void active_list_queue_for_remove_after(ChaosMachine* machine, ActiveChaosEffectList* element) {
    ActiveChaosEffectList* removed;
    if (element == NULL) {
        removed = machine->active_effects;
        machine->active_effects = removed->next;
    } else {
        removed = element->next;
        element->next = removed->next;
    }

    removed->next = machine->remove_queue;
    machine->remove_queue = removed;
}

static void active_list_queue_for_remove_entity(ChaosMachine* machine, ChaosGroup* group, ChaosEffectEntity* entity) {
    ActiveChaosEffectList* prev = NULL;

    for (ActiveChaosEffectList* cur = machine->active_effects; cur != NULL; cur = cur->next) {
        if (cur->effect == entity) {
            active_list_queue_for_remove_after(machine, prev);
        }

        prev = cur;
    }
}

static void active_list_add(ChaosMachine* machine, ChaosGroup* group, ChaosEffectEntity* entity, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* new = recomp_alloc(sizeof(ActiveChaosEffectList));
    if (new == NULL) {
        error("Couldn't alloc an active list element!");
        return;
    }

    new->effect = entity;
    new->group = group;
    new->timer = 0;
    new->next = machine->active_effects;
    machine->active_effects = new;

    if (entity->status == CHAOS_EFFECT_STATUS_AVAILABLE) {
        entity->status = CHAOS_EFFECT_STATUS_ACTIVE;
        update_weight_sums_upwards(group, entity);
    }

    ChaosEffect* effect = &entity->effect;
    effect_start(effect, gfxCtx, gameState);
}

static void active_list_remove_after(ChaosMachine* machine, ChaosGroup* group, ActiveChaosEffectList* element, GraphicsContext* gfxCtx, GameState* gameState) {
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
    update_weight_sums_upwards(group, entity);

    effect_end(effect, gfxCtx, gameState);

    recomp_free(del);
}


static void active_list_update(ChaosMachine* machine, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* prev = NULL;

    for (ActiveChaosEffectList* cur = machine->active_effects; cur != NULL; cur = cur->next) {
        ChaosEffectEntity* entity = cur->effect;
        ChaosEffect* effect = &entity->effect;

        effect_update(effect, gfxCtx, gameState);

        if (cur->timer >= effect->duration) {
            ChaosGroup* group = cur->group;
            active_list_remove_after(machine, group, prev, gfxCtx, gameState);
            continue;
        }
        cur->timer++;

        prev = cur;
    }
}

static void remove_list_process(ChaosMachine* machine, GraphicsContext* gfxCtx, GameState* gameState) {
    ActiveChaosEffectList* prev = NULL;

    ActiveChaosEffectList* cur = machine->remove_queue;
    while (cur != NULL) {
        ChaosEffectEntity* entity = cur->effect;
        ChaosEffect* effect = &entity->effect;

        effect_update(effect, gfxCtx, gameState);
        effect_end(effect, gfxCtx, gameState);

        ActiveChaosEffectList* tmp = cur;
        cur = cur->next;
        recomp_free(tmp);
    }

    machine->remove_queue = NULL;
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
    f32 rand = Rand_ZeroOne() * get_available_weight_sum(group);

    ChaosEffectEntity* choice = get_effect_entity_by_weight(group, rand);

    if (effect_count > 1) {
        f32 weight_share = get_effect_entity_weight(group, choice) * group->settings.winner_weight_share;
        f32 weight_share_per_effect = weight_share / (effect_count - 1);
        group->shared_weight += weight_share_per_effect;
        choice->weight_modifier -= weight_share + weight_share_per_effect;
    }

    return choice;
}

static void machine_perform_roll(ChaosMachine* machine, GraphicsContext* gfxCtx, GameState* gameState) {
    debug_log("Beginning roll in '%s' chaos machine.", machine->settings.name);

    ChaosGroup* group = pick_chaos_group(machine);

    if (group != NULL) {
        ChaosDisturbance disturbance = get_group_disturbance(machine, group);
        debug_log("Selected %s disturbance group.\n\tGroup's probability after selection: %f.", DISTURBANCE_NAME[disturbance], group->probability);

        ChaosEffectEntity* effect = pick_chaos_effect(group);
        debug_log("Selected '%s' effect.\n\tEffect's weight after selection: %f.", effect->effect.name, get_effect_entity_weight(group, effect));

        if (effect != NULL) {
            active_list_add(machine, group, effect, gfxCtx, gameState);
        }
    } else {
        debug_log("Roll landed on empty space.");
    }

    debug_log("Roll finished.");
}

static void machine_update(ChaosMachine* machine, GraphicsContext* gfxCtx, GameState* gameState) {
    u32 cycle_length = machine->settings.cycle_length;
    if (cycle_length > 0) {
        machine->cycle_timer++;
        if (machine->cycle_timer >= cycle_length) {
            machine_perform_roll(machine, gfxCtx, gameState);
            machine->cycle_timer = 0;
        }
    }

    while(machine->roll_requests > 0) {
        machine_perform_roll(machine, gfxCtx, gameState);
        machine->roll_requests--;
    }

    active_list_update(machine, gfxCtx, gameState);
    remove_list_process(machine, gfxCtx, gameState);
}

void chaos_update(GraphicsContext* gfxCtx, GameState* gameState) {
    for (u32 i = 0; i < machine_count; i++) {
        machine_update(&machines[i], gfxCtx, gameState);
    }
}

RECOMP_EXPORT void chaos_enable_effect(ChaosEffectEntity* entity) {
    if (state != CHAOS_STATE_RUN) {
        warning("Chaos effects can't be enabled before initialization!");
    }

    // TODO Check tags.
    if (entity->status == CHAOS_EFFECT_STATUS_DISABLED) {
        ChaosMachine* machine = find_machine(entity);
        ChaosGroup* group = find_group(machine, entity);

        entity->status = CHAOS_EFFECT_STATUS_AVAILABLE;
        update_weight_sums_upwards(group, entity);

        debug_log("Enabled '%s' effect in '%s' chaos machine.", entity->effect.name, machine->settings.name);
    }
}

RECOMP_EXPORT void chaos_disable_effect(ChaosEffectEntity* entity) {
    if (state != CHAOS_STATE_RUN) {
        warning("Chaos effects can't be disabled before initalization!");
    }

    ChaosMachine* machine = find_machine(entity);
    ChaosGroup* group = find_group(machine, entity);

    // TODO Check tags.
    switch (entity->status) {
        case CHAOS_EFFECT_STATUS_AVAILABLE:
            entity->status = CHAOS_EFFECT_STATUS_DISABLED;
            update_weight_sums_upwards(group, entity);
            break;
        case CHAOS_EFFECT_STATUS_ACTIVE:
            entity->status = CHAOS_EFFECT_STATUS_DISABLED;
            active_list_queue_for_remove_entity(machine, group, entity);
            break;
        default:
            return;
    }

    debug_log("Disabled '%s' effect in '%s' chaos machine.", entity->effect.name, machine->settings.name);
}


RECOMP_EXPORT void chaos_request_roll(ChaosMachine* machine) {
    if (state != CHAOS_STATE_RUN) {
        warning("Can't request chaos effect rolls before initalization!");
    }

    machine->roll_requests++;

    debug_log("Requested roll in '%s' chaos machine.", machine->settings.name);
}