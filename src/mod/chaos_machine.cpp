#include "chaos.h"

#include <memory>
#include <cstring>

namespace Chaos {
    ChaosMachine::ChaosMachine(const ChaosMachineSettings& settings)
    : settings(settings) {
        for (int i = 0; i < Disturbance::MAX; i++) {
            groups.emplace_back(settings.default_groups_settings[i]);
        }
        std::memset(group_roll_requests, 0, Disturbance::MAX * sizeof(u8));
    }

    ChaosMachineSettings& ChaosMachine::get_settings() {
        return settings;
    }

    Disturbance ChaosMachine::get_group_disturbance(ChaosGroup* group) const {
        return static_cast<Disturbance>(group - &groups[0]);
    }

    ChaosGroup& ChaosMachine::get_group(Disturbance disturbance) {
        return groups[disturbance];
    }

    ChaosGroup* ChaosMachine::pick_group() {
        f32 rand = Rand_ZeroOne();

        for (int i = 0; i < Disturbance::MAX; i++) {
            ChaosGroup& group = groups[i];
            f32 group_probability = group.get_probability();

            if ((rand < group_probability) && (group.get_effect_count() > 0)) {
                group.apply_on_pick_multiplier();
                return &group;
            }
            rand -= group_probability;
        }

        return nullptr;
    }

    void ChaosMachine::perform_roll(ChaosGroup& group) {
        ChaosEffectEntity& effect = group.pick_effect();

        debug_log("Selected '%s' effect.\n\tEffect's weight after selection: %f.",
            effect.effect.name, group.get_effect_weight(effect));
        active_effects.add(group, effect);
    }

    void ChaosMachine::perform_roll(Disturbance disturbance) {
        ChaosGroup& group = groups[disturbance];
        perform_roll(group);
    }

    void ChaosMachine::perform_roll() {
        debug_log("Beginning roll in '%s' chaos machine.", settings.name);

        ChaosGroup* group = pick_group();

        if (group != nullptr) {
            Disturbance disturbance = get_group_disturbance(group);
            debug_log("Selected %s disturbance group.\n\tGroup's probability after selection: %f.",
                DISTURBANCE_NAME[disturbance], group->get_probability());

            perform_roll(*group);
        } else {
            debug_log("Roll landed on empty space.");
        }

        debug_log("Roll finished.");
    }

    void ChaosMachine::update() {
        u32 cycle_length = debug_disable_rolling ? 0 : settings.cycle_length;
        if (cycle_length > 0) {
            cycle_timer++;
            if (cycle_timer >= cycle_length) {
                perform_roll();
                cycle_timer = 0;
            }
        }

        while(roll_requests > 0) {
            perform_roll();
            roll_requests--;
        }

        for (int i = 0; i < Disturbance::MAX; i++) {
            while(group_roll_requests[i] > 0) {
                debug_log("Beginning group roll in '%s' chaos machine's %s disturbance group.",
                    settings.name, DISTURBANCE_NAME[i]);

                ChaosGroup& group = groups[i];
                perform_roll(group);

                debug_log("Group roll finished.");

                group_roll_requests[i]--;
            }
        }

        active_effects.update();
        active_effects.empty_remove_queue();
    }

    void ChaosMachine::enable_effect(ChaosEffectEntity& entity) {
        ChaosGroup& group = *entity.owner;

        if (!Tag::is_combo_allowed(entity.combo)) {
            error("Can't enable '%s' effect because of a tag conflict.",
                entity.effect.name);
            return;
        }

        if (entity.status == ChaosEffectStatus::DISABLED) {
            group.set_effect_status(entity, ChaosEffectStatus::AVAILABLE);

            debug_log("Enabled '%s' effect in '%s' chaos machine.",
                entity.effect.name, settings.name);
        }
    }

    void ChaosMachine::disable_effect(ChaosEffectEntity& entity) {
        ChaosGroup& group = *entity.owner;

        // TODO Check tags.
        switch (entity.status) {
            case ChaosEffectStatus::AVAILABLE:
                group.set_effect_status(entity, ChaosEffectStatus::DISABLED);
                break;
            case ChaosEffectStatus::ACTIVE:
                group.set_effect_status(entity, ChaosEffectStatus::DISABLED);
                active_effects.queue_for_remove_entity(entity);
                break;
            default:
                return;
        }

        debug_log("Disabled '%s' effect in '%s' chaos machine.",
            entity.effect.name, settings.name);
    }

    void ChaosMachine::stop_effect(ChaosEffectEntity& entity) {
        ChaosGroup& group = *entity.owner;

        switch (entity.status) {
            case ChaosEffectStatus::ACTIVE:
                // TODO Check tags.
                active_effects.queue_for_remove_entity(entity);
                debug_log("Stopped '%s' effect in '%s' chaos machine.",
                    entity.effect.name, settings.name);
                break;
            default:
                break;
        }
    }
}