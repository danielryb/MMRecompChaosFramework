#include "chaos.h"
#include "tag_names.h"
#include "util/static_vector.h"
#include "util/finite_vector.h"

#include <memory>
#include <cstring>

namespace Chaos {
    constexpr int FRAMES_PER_SECOND = 20;

    constexpr int INITIAL_MACHINE_COUNT = 1;

    const char* DISTURBANCE_NAME[Disturbance::MAX] = {
        "VERY_LOW",
        "LOW",
        "MEDIUM",
        "HIGH",
        "VERY_HIGH",
        "NIGHTMARE",
    };

    GameCtx* _ctx = NULL;

    bool debug_disable_rolling = false;

    inline constexpr ChaosMachineSettings DEFAULT_MACHINE_SETTINGS {
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

    RECOMP_DECLARE_EVENT(chaos_on_init());


    enum State : int {
        DEFAULT,
        MACHINE_COUNT,
        MACHINE_REGISTER,
        EFFECT_COUNT,
        EFFECT_REGISTER,
        RUN,
    };

    State state;

    std::unique_ptr<finite_vector<ChaosMachine>> machines;
    u32 machine_count = 0;

    std::unordered_set<ChaosEffect*> pause_fun_queue;
    std::unordered_set<ChaosEffect*> unpause_fun_queue;

    void alloc_effect_slots() {
        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                group.alloc_effect_slots();
            }
        }
    }

    void reset_effect_counts() {
        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                group.reset_effect_count();
            }
        }
    }

    void call_init_callback() {
        reset_effect_counts();
        machine_count = 0;

        register_machine(DEFAULT_MACHINE_SETTINGS);
        register_tag(CHAOS_TAG_PLAYER_INACTIVE, SIZE_MAX);
        register_tag(CHAOS_TAG_CUTSCENE, SIZE_MAX);

        chaos_on_init();
    }

    ChaosMachine& get_machine(size_t pos) {
        return (*machines)[pos];
    }

    ChaosMachine& get_machine(ChaosGroup& group) {
        u32 pos = (reinterpret_cast<u8*>(&group)
            - reinterpret_cast<u8*>(&(*machines)[0])) / sizeof(ChaosMachine);
        return (*machines)[pos];
    }

    ChaosMachine* get_machine_or_null(size_t pos) noexcept {
        if (!machines || pos >= machines->size()) {
            return nullptr;
        }
        return &(*machines)[pos];
    }


    void register_tag(const char* tag, size_t limit) {
        if ((state == State::RUN) || (state == State::DEFAULT)) {
            warning("Reservation limit can be changed only during the initalization!");
            return;
        } else if (state > State::MACHINE_COUNT) {
            return;
        }

        if (!Tag::add_tag(tag, limit)) {
            error("Tag '%s' has already been defined!", tag);
        }
    }

    ChaosMachine* register_machine(const ChaosMachineSettings& settings) {
        ChaosMachine* ret = nullptr;
        switch (state) {
            case State::MACHINE_REGISTER: {
                machines->emplace_back(settings);

                machine_count++;
                ChaosMachine& machine = (*machines)[machine_count];

                ret = &machine;
                break;
            }
            case State::RUN:
            case State::DEFAULT: {
                warning("Chaos machines can only be registered as callbacks to 'chaos_on_init'!");
                break;
            }
            default: {
                ret = &(*machines)[machine_count];
            case State::MACHINE_COUNT:
                machine_count++;
                break;
            }
        }
        return ret;
    }

    ChaosEffectEntity* register_effect(ChaosMachine* machine, const ChaosEffect& effect,
            Disturbance disturbance, const char* tag_names[], size_t tag_count) {
        if (disturbance >= Disturbance::MAX) {
            warning("Invalid disturbance provided!");
            return NULL;
        }

        switch (state) {
            case State::EFFECT_COUNT: {
                ChaosGroup& group = machine->get_group(disturbance);
                Tag::combo_id combo = Tag::get_combo_id(tag_names, tag_count);
                group.reserve_effect_slot(combo);
                break;
            }
            case State::EFFECT_REGISTER: {
                ChaosGroup& group = machine->get_group(disturbance);
                Tag::combo_id combo = Tag::get_combo_id(tag_names, tag_count);
                u32 i = group.reserve_effect_slot(combo);
                ChaosEffectEntity& entity = group.get_effect(combo, i);

                entity.effect = effect;
                entity.status = ChaosEffectStatus::AVAILABLE;
                entity.owner = &group;
                entity.combo = combo;

                return &entity;
            }
            case State::RUN:
            case State::DEFAULT: {
                warning("Chaos effects can only be registered as callbacks to 'chaos_on_init'!");
                break;
            }
            default: {
                break;
            }
        }

        return NULL;
    }


    void init() {
        Tag::clear();

        state = State::MACHINE_COUNT;

        call_init_callback();

        machines = std::make_unique<finite_vector<ChaosMachine>>(machine_count);
        if (machines == nullptr) {
            error("Couldn't allocate an array for chaos machines!");
            return;
        }

        debug_log("Detected %d chaos machine registration%s.", machine_count,
            ((machine_count != 1) ? "s" : ""));

        state = State::MACHINE_REGISTER;

        call_init_callback();

        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            debug_log("Created '%s' chaos machine.", machine.get_settings().name);
        }

        state = State::EFFECT_COUNT;

        call_init_callback();

        alloc_effect_slots();

        for (u32 i = 0; i < machine_count; i++) {
            u32 total_count = 0;

            ChaosMachine& machine = (*machines)[i];
            ChaosMachineSettings& machine_settings = machine.get_settings();
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                total_count += group.size();
            }

            debug_log("Detected %d chaos effect registration%s to '%s'.",
                total_count, ((total_count != 1) ? "s" : ""), machine.get_settings().name);
        }

        state = State::EFFECT_REGISTER;

        call_init_callback();

        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                group.init_tree();

                u32 effect_count = group.get_effect_count();

                // TODO Reimplement
                // for (u32 k = 0; k < effect_count; k++) {
                //     ChaosEffectEntity& entity = group.get_effect(k);

                //     debug_log("Registered '%s' effect to '%s' chaos machine with %s disturbance.",
                //         entity.effect.name, machine.get_settings().name, DISTURBANCE_NAME[j]);
                // }
            }
        }

        state = State::RUN;
    }

    void update(GameCtx* ctx) {
        _ctx = ctx;

        for (u32 i = 0; i < get_machine_count(); i++) {
            ChaosMachine& machine = get_machine(i);
            machine.update();
        }
    }


    void enable_effect(ChaosEffectEntity& entity) {
        if (state < State::RUN) {
            warning("Chaos effects can't be enabled before initialization!");
        }

        ChaosGroup& group = *entity.owner;
        ChaosMachine& machine = get_machine(group);

        machine.enable_effect(entity);
    }

    void disable_effect(ChaosEffectEntity& entity) {
        if (state < State::RUN) {
            warning("Chaos effects can't be disabled before initalization!");
        }

        ChaosGroup& group = *entity.owner;
        ChaosMachine& machine = get_machine(group);

        machine.disable_effect(entity);
    }

    void activate_effect(ChaosEffectEntity& entity) {
        if (state < State::RUN) {
            warning("Chaos effects can't be activated before initalization!");
        }

        ChaosGroup& group = *entity.owner;
        ChaosMachine& machine = get_machine(group);

        machine.activate_effect(entity);
    }

    void stop_effect(ChaosEffectEntity& entity) {
        if (state < State::RUN) {
            warning("Chaos effects can't be stopped before initalization!");
        }

        ChaosGroup& group = *entity.owner;
        ChaosMachine& machine = get_machine(group);

        machine.stop_effect(entity);
    }


    void forbid_tag(const char* tag) {
        if (state < State::RUN) {
            warning("Tags can't be forbidden before initalization!");
        }

        Tag::tag_id id = Tag::get_tag_id(tag);
        auto [res, affected] = Tag::exclude_tag(id);
        if (!res) {
            return;
        }
        deactivate_subgroups(affected);

        auto& related = Tag::get_related_combos(id);
        std::unordered_set<Tag::combo_id> pausable(related.begin(), related.end());

        for (size_t i = 0; i < machines->size(); i++) {
            auto& machine = (*machines)[i];
            machine.pause_effects(pausable);
        }
    }

    void allow_tag(const char* tag) {
        if (state < State::RUN) {
            warning("Tags can't be allowed before initalization!");
        }

        Tag::tag_id id = Tag::get_tag_id(tag);
        auto [res, affected] = Tag::include_tag(id);
        if (!res) {
            return;
        }
        activate_subgroups(affected);

        std::unordered_set<Tag::combo_id> reasumable;
        auto& related = Tag::get_related_combos(id);
        for (auto combo : related) {
            if (Tag::is_combo_included(combo)) {
                reasumable.insert(combo);
            }
        }

        for (size_t i = 0; i < machines->size(); i++) {
            auto& machine = (*machines)[i];
            machine.unpause_effects(reasumable);
        }
    }


    void request_roll(ChaosMachine& machine, double group_rand, double effect_rand) {
        if (state < State::RUN) {
            warning("Can't request chaos effect rolls before initalization!");
        }

        machine.perform_roll(group_rand, effect_rand);

        debug_log("Requested roll in '%s' chaos machine.", machine.get_settings().name);
    }

    void request_roll(ChaosMachine& machine, Disturbance disturbance, double rand) {
        if (state < State::RUN) {
            warning("Can't request chaos effect rolls before initalization!");
        }

        machine.perform_roll(disturbance, rand);

        debug_log("Requested roll in %s disturbance group in '%s' chaos machine.",
            machine.get_settings().name);
    }


    size_t get_machine_count() {
        return machine_count;
    }

    u32 get_total_effect_count() {
        u32 num_effects = 0;

        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                num_effects += group.size();
            }
        }

        return num_effects;
    }


    void activate_subgroups(const std::unordered_set<Tag::combo_id>& subgroups) {
        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                for (Tag::combo_id combo : subgroups) {
                    group.activate_subgroup(combo);
                }
            }
        }
    }

    void deactivate_subgroups(const std::unordered_set<Tag::combo_id>& subgroups) {
        for (u32 i = 0; i < machine_count; i++) {
            ChaosMachine& machine = (*machines)[i];
            for (int j = 0; j < Disturbance::MAX; j++) {
                ChaosGroup& group = machine.get_group(Disturbance(j));
                for (Tag::combo_id combo : subgroups) {
                    group.deactivate_subgroup(combo);
                }
            }
        }
    }


    void queue_pause_fun(ChaosEffect* effect) {
        if (!unpause_fun_queue.erase(effect)) {
            pause_fun_queue.insert(effect);
        }
    }

    void queue_unpause_fun(ChaosEffect* effect) {
        if (!pause_fun_queue.erase(effect)) {
            unpause_fun_queue.insert(effect);
        }
    }

    void execute_fun_queues() {
        for (ChaosEffect* effect : pause_fun_queue) {
            effect->on_pause_fun(_ctx);
        }
        pause_fun_queue.clear();

        for (ChaosEffect* effect : unpause_fun_queue) {
            effect->on_unpause_fun(_ctx);
        }
        unpause_fun_queue.clear();
    }



    void chaos_init() {
        init();
    }

    void chaos_update(GameCtx* ctx) {
        update(ctx);
    }

    void chaos_execute_fun_queues() {
        execute_fun_queues();
    }


    RECOMP_EXPORT void chaos_register_tag(const char* tag, size_t limit) {
        register_tag(tag, limit);
    }

    RECOMP_EXPORT ChaosMachine* chaos_register_machine(const ChaosMachineSettings* settings) {
        return register_machine(*settings);
    }

    RECOMP_EXPORT ChaosEffectEntity* chaos_register_effect_to(
            ChaosMachine* machine, const ChaosEffect* effect,
            Disturbance disturbance, const char* tag_names[], size_t tag_count) {
        return register_effect(machine, *effect, disturbance, tag_names, tag_count);
    }

    RECOMP_EXPORT ChaosEffectEntity* chaos_register_effect(
            const ChaosEffect* effect, Disturbance disturbance,
            const char* tag_names[], size_t tag_count) {
        return register_effect(
            get_machine_or_null(0), *effect, disturbance, tag_names, tag_count);
    }


    RECOMP_EXPORT void chaos_activate_effect(ChaosEffectEntity* entity) {
        activate_effect(*entity);
    }

    RECOMP_EXPORT void chaos_enable_effect(ChaosEffectEntity* entity) {
        enable_effect(*entity);
    }

    RECOMP_EXPORT void chaos_disable_effect(ChaosEffectEntity* entity) {
        disable_effect(*entity);
    }

    RECOMP_EXPORT void chaos_stop_effect(ChaosEffectEntity* entity) {
        stop_effect(*entity);
    }


    RECOMP_EXPORT void chaos_request_roll(ChaosMachine* machine) {
        request_roll(*machine);
    }

    RECOMP_EXPORT void chaos_request_roll_d(
        ChaosMachine* machine, double group_rand, double effect_rand) {
        request_roll(*machine, group_rand, effect_rand);
    }

    RECOMP_EXPORT void chaos_request_group_roll(
        ChaosMachine* machine, Disturbance disturbance) {
        request_roll(*machine, disturbance);
    }

    RECOMP_EXPORT void chaos_request_group_roll_d(
        ChaosMachine* machine, Disturbance disturbance, double rand) {
        request_roll(*machine, disturbance, rand);
    }


    RECOMP_EXPORT void chaos_forbid_tag(const char* tag) {
        forbid_tag(tag);
    }

    RECOMP_EXPORT void chaos_allow_tag(const char* tag) {
        allow_tag(tag);
    }
}
