#include "chaos.h"
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
            Disturbance disturbance, const char** tagnames, size_t tagcount) {
        if (disturbance >= Disturbance::MAX) {
            warning("Invalid disturbance provided!");
            return NULL;
        }

        switch (state) {
            case State::EFFECT_COUNT: {
                ChaosGroup& group = machine->get_group(disturbance);
                Tag::combo_id combo = Tag::get_combo_id(tagnames, tagcount);
                group.reserve_effect_slot(combo);
                break;
            }
            case State::EFFECT_REGISTER: {
                ChaosGroup& group = machine->get_group(disturbance);
                Tag::combo_id combo = Tag::get_combo_id(tagnames, tagcount);
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
                total_count += group.get_effect_count();
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

    void stop_effect(ChaosEffectEntity& entity) {
        if (state < State::RUN) {
            warning("Chaos effects can't be stopped before initalization!");
        }

        ChaosGroup& group = *entity.owner;
        ChaosMachine& machine = get_machine(group);

        machine.stop_effect(entity);
    }

    void request_roll(ChaosMachine& machine) {
        if (state < State::RUN) {
            warning("Can't request chaos effect rolls before initalization!");
        }

        machine.perform_roll();

        debug_log("Requested roll in '%s' chaos machine.", machine.get_settings().name);
    }

    void request_roll(ChaosMachine& machine, Disturbance disturbance) {
        if (state < State::RUN) {
            warning("Can't request chaos effect rolls before initalization!");
        }

        machine.perform_roll(disturbance);

        debug_log("Requested roll in %s disturbance group in '%s' chaos machine.",
            machine.get_settings().name);
    }

    size_t get_machine_count() {
        return machine_count;
    }

    void activate_subgroups(std::unordered_set<Tag::combo_id> subgroups) {
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

    void deactivate_subgroups(std::unordered_set<Tag::combo_id> subgroups) {
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



    void chaos_init() {
        init();
    }

    void chaos_update(GameCtx* ctx) {
        _ctx = ctx;

        for (u32 i = 0; i < get_machine_count(); i++) {
            ChaosMachine& machine = get_machine(i);
            machine.update();
        }
    }


    RECOMP_EXPORT ChaosMachine* chaos_register_machine(const ChaosMachineSettings* settings) {
        return register_machine(*settings);
    }

    RECOMP_EXPORT ChaosEffectEntity* chaos_register_effect_to(
            ChaosMachine* machine, const ChaosEffect* effect,
            Disturbance disturbance, const char** tag_names, size_t tag_count) {
        return register_effect(machine, *effect, disturbance, tag_names, tag_count);
    }

    RECOMP_EXPORT ChaosEffectEntity* chaos_register_effect(
            const ChaosEffect* effect, Disturbance disturbance,
            const char** tag_names, size_t tag_count) {
        return register_effect(
            get_machine_or_null(0), *effect, disturbance, tag_names, tag_count);
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

    RECOMP_EXPORT void chaos_request_group_roll(ChaosMachine* machine, Disturbance disturbance) {
        request_roll(*machine, disturbance);
    }
}
