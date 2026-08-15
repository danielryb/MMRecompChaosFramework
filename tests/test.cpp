#include "chaos.h"
#include "events.h"

#include <iostream>
#include <cassert>
#include <format>

#define _countof(arr) sizeof(arr) / sizeof(arr[0]);

using namespace Chaos;

inline void reserve_slot(Chaos::ChaosGroup& group, const char* tag_names[], int tag_count) {
    Tag::combo_id combo = Tag::get_combo_id(tag_names, tag_count);
    group.reserve_effect_slot(combo);
}

inline void add_entity(Chaos::ChaosGroup& group, const char* tag_names[], int tag_count) {
    Tag::combo_id combo = Tag::get_combo_id(tag_names, tag_count);
    u32 i = group.reserve_effect_slot(combo);
    ChaosEffectEntity& entity = group.get_effect(combo, i);

    entity.owner = &group;
    entity.combo = combo;
}

/**
 * Tests allocation of a simple chaos group and a single effect
 * draw.
*/
void test_tree_weights() {
    ChaosGroup group({ /* CHAOS_DISTURBANCE_VERY_LOW */
        .initial_probability = 0.3f,
        .on_pick_multiplier = 1.0f,
        .winner_weight_share = 0.2f,
    });

    for (int i = 0; i < 5; i++) {
        reserve_slot(group, NULL, 0);
    }

    group.alloc_effect_slots();

    group.reset_effect_count();

    for (int i = 0; i < 5; i++) {
        add_entity(group, NULL, 0);
    }

    group.init_tree();

    group.pick_effect_by_weight(4.0);

    assert(group.get_weight_sum() == 5.0);
}

/**
 * Tests if the overall sum of weights in the chaos group stays
 * roughly the same over subsequent weight redistributions.
*/
void test_weight_balance() {
    constexpr int EFFECT_COUNT = 100;
    constexpr double EPSILON = 0.000001;

    const char* tags1[] = { "tag1" };
    constexpr size_t tag_count1 = _countof(tags1);

    const char* tags2[] = { "tag2" };
    constexpr size_t tag_count2 = _countof(tags2);

    const char* tags12[] = { "tag1", "tag2" };
    constexpr size_t tag_count12 = _countof(tags12);

    const char** tag_groups[] = { tags1, tags2, tags12 };
    const size_t tag_counts[] = { tag_count1, tag_count2, tag_count12 };
    constexpr size_t GROUP_COUNT = _countof(tag_groups);

    ChaosGroup group({ /* CHAOS_DISTURBANCE_VERY_LOW */
        .initial_probability = 0.3f,
        .on_pick_multiplier = 1.0f,
        .winner_weight_share = 0.2f,
    });

    for (size_t j = 0; j < GROUP_COUNT; j++) {
        const char** tag_group = tag_groups[j];
        size_t tag_count = tag_counts[j];

        for (int i = 0; i < EFFECT_COUNT; i++) {
            reserve_slot(group, tag_group, tag_count);
        }
    }

    group.alloc_effect_slots();

    group.reset_effect_count();

    for (size_t j = 0; j < GROUP_COUNT; j++) {
        const char** tag_group = tag_groups[j];
        size_t tag_count = tag_counts[j];

        for (int i = 0; i < EFFECT_COUNT; i++) {
            add_entity(group, tag_group, tag_count);
        }
    }

    group.init_tree();

    for (int i = 0; i < 50000; i++) {
        group.pick_effect();
    }

    assert(group.get_weight_sum() - EFFECT_COUNT * GROUP_COUNT < EPSILON);
}

/**
 * Tests if group updates properly when the status of it's node changes.
*/
void test_status_change() {
    constexpr int EFFECT_COUNT = 100;

    const char* tags1[] = { "tag1" };
    constexpr size_t tag_count1 = _countof(tags1);

    const char* tags2[] = { "tag2" };
    constexpr size_t tag_count2 = _countof(tags2);

    const char* tags12[] = { "tag1", "tag2" };
    constexpr size_t tag_count12 = _countof(tags12);

    const char* tags123[] = { "tag1", "tag2", "tag3" };
    constexpr size_t tag_count123 = _countof(tags123);

    const char** tag_groups[] = { tags1, tags2, tags12, tags123 };
    const size_t tag_counts[] = { tag_count1, tag_count2, tag_count12, tag_count123 };
    constexpr size_t GROUP_COUNT = _countof(tag_groups);

    constexpr int TOTAL_EFFECT_COUNT = EFFECT_COUNT * GROUP_COUNT;

    constexpr const ChaosEffect test_effect = {
        .name = "test",
        .duration = 0,

        .on_start_fun = NULL,
        .update_fun = NULL,
        .on_end_fun = NULL,
        .on_pause_fun = NULL,
        .on_unpause_fun = NULL,
    };

    Chaos::set_on_init([&]() {
        ChaosMachine* machine = Chaos::get_machine_or_null(0);

        for (size_t j = 0; j < GROUP_COUNT; j++) {
            const char** tag_group = tag_groups[j];
            size_t tag_count = tag_counts[j];

            for (int i = 0; i < EFFECT_COUNT; i++) {
                Chaos::register_effect(
                    machine, test_effect, Disturbance::VERY_LOW, tag_group, tag_count);
            }
        }
    });

    Chaos::init();

    ChaosMachine& machine = Chaos::get_machine(0);
    ChaosGroup& group = machine.get_group(Disturbance::VERY_LOW);

    Tag::combo_id combo1 = Tag::get_combo_id(tags1, tag_count1);
    ChaosEffectEntity& effect1 = group.get_effect(combo1, 0);

    Tag::combo_id combo2 = Tag::get_combo_id(tags2, tag_count2);
    ChaosEffectEntity& effect2 = group.get_effect(combo2, 0);

    Tag::combo_id combo12 = Tag::get_combo_id(tags12, tag_count12);
    ChaosEffectEntity& effect12 = group.get_effect(combo12, 0);


    constexpr int EFFECT_COUNT_3_DISABLED = TOTAL_EFFECT_COUNT - 3 * EFFECT_COUNT;

    group.set_effect_status(effect1, ChaosEffectStatus::ACTIVE);
    assert(group.get_weight_sum() == EFFECT_COUNT_3_DISABLED);



    group.set_effect_status(effect2, ChaosEffectStatus::ACTIVE);
    assert(group.get_weight_sum() == 0);

    group.set_effect_status(effect2, ChaosEffectStatus::DISABLED);
    assert(group.get_weight_sum() == EFFECT_COUNT_3_DISABLED - 1);

    group.set_effect_status(effect2, ChaosEffectStatus::HIDDEN);
    assert(group.get_weight_sum() == EFFECT_COUNT_3_DISABLED - 1);

    group.set_effect_status(effect2, ChaosEffectStatus::AVAILABLE);
    assert(group.get_weight_sum() == EFFECT_COUNT_3_DISABLED);



    group.set_effect_status(effect1, ChaosEffectStatus::DISABLED);
    assert(group.get_weight_sum() == TOTAL_EFFECT_COUNT - 1);

    group.set_effect_status(effect1, ChaosEffectStatus::HIDDEN);
    assert(group.get_weight_sum() == TOTAL_EFFECT_COUNT - 1);

    group.set_effect_status(effect1, ChaosEffectStatus::AVAILABLE);
    assert(group.get_weight_sum() == TOTAL_EFFECT_COUNT);



    group.set_effect_status(effect12, ChaosEffectStatus::ACTIVE);
    assert(group.get_weight_sum() == 0);

    group.set_effect_status(effect12, ChaosEffectStatus::DISABLED);
    assert(group.get_weight_sum() == TOTAL_EFFECT_COUNT - 1);

    group.set_effect_status(effect12, ChaosEffectStatus::HIDDEN);
    assert(group.get_weight_sum() == TOTAL_EFFECT_COUNT - 1);

    group.set_effect_status(effect12, ChaosEffectStatus::AVAILABLE);
    assert(group.get_weight_sum() == TOTAL_EFFECT_COUNT);
}

int main(int argc, const char** argv) {
    test_tree_weights();
    test_weight_balance();
    test_status_change();

    return 0;
}