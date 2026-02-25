#include "chaos.h"

#include <memory>
#include <cstring>

namespace Chaos {
    f32 ChaosGroup::EffectSubtree::get_weight(Node& node) {
        return node.weight_deviation + owner.shared_weight;
    }

    f32 ChaosGroup::EffectSubtree::get_left_weight(Node& node) {
        return node.left_deviation_sum + node.left_count * owner.shared_weight;
    }


    size_t ChaosGroup::EffectSubtree::reserve_slot() {
        return size_++;
    }

    void ChaosGroup::EffectSubtree::reset_size() {
        size_ = 0;
    }

    void ChaosGroup::EffectSubtree::alloc_nodes() {
        nodes = std::make_unique<Node[]>(size_);
    }


    bool ChaosGroup::EffectSubtree::is_counted(Node& node) {
        return (node.effect.status == ChaosEffectStatus::AVAILABLE);
    }

    f32 ChaosGroup::EffectSubtree::get_deviation(Node& node) {
        if (is_counted(node)) {
            return node.weight_deviation;
        }
        return 0.0;
    }

    void ChaosGroup::EffectSubtree::init_tree() {
        for (size_t i = size_; i > 0; i--) {
            Node& node = nodes[i - 1];

            f32 deviation = get_deviation(node);
            size_t c = is_counted(node) ? 1 : 0;

            bool last_child_left = (i % 2 == 0);

            for (size_t j = i / 2; j > 0; j /= 2) {
                Node& parent = nodes[j - 1];

                if (last_child_left) {
                    parent.left_deviation_sum += deviation;
                    parent.left_count += c;
                }
                last_child_left = (i % 2 == 0);
            }

            deviation_sum += deviation;
            count += c;
        }
    }

    void ChaosGroup::EffectSubtree::update_deviations_upwards(Node& start, f32 delta) {
        size_t i = get_pos(start) + 1;

        bool last_child_left = (i % 2 == 0);
        for (size_t j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];

            if (last_child_left) {
                parent.left_deviation_sum += delta;
            }
            last_child_left = (i % 2 == 0);
        }

        deviation_sum += delta;
    }

    void ChaosGroup::EffectSubtree::update_counts_upwards(Node& start, size_t delta) {
        size_t i = get_pos(start) + 1;

        bool last_child_left = (i % 2 == 0);
        for (size_t j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];

            if (last_child_left) {
                parent.left_count += delta;
            }
            last_child_left = (i % 2 == 0);
        }

        count += delta;
    }


    ChaosGroup::EffectSubtree::Node& ChaosGroup::EffectSubtree::get_node(f32 weight) {
        for (size_t i = 1; i <= size_;) {
            Node& node = nodes[i - 1];
            if (get_left_weight(node) >= weight) {
                i = i * 2;
            } else if (get_left_weight(node) + get_weight(node) >= weight) {
                return node;
            } else {
                i = i * 2 + 1;
            }
        }

        // Shouldn't happen
        error("get_effect_entity_by_weight didn't find any node within given weight limit. "
              "Returning first node.");
        return nodes[0];
    }

    size_t ChaosGroup::EffectSubtree::get_pos(Node& node) {
        return &node - nodes.get();
    }



    size_t ChaosGroup::EffectTree::size() {
        if (combo_subgroups.size() == 0) {
            return 0;
        }
        return combo_subgroups.size() * 2 - 1;
    }

    f32 ChaosGroup::EffectTree::get_weight(EffectSubtree::Node& node) {
        return node.weight_deviation + shared_weight;
    }

    f32 ChaosGroup::EffectTree::get_weight_sum() {
        return deviation_sum + count * shared_weight;
    }

    f32 ChaosGroup::EffectTree::get_left_weight(Node& node) {
        Info& info = node.info;
        return info.left_deviation_sum + info.left_count * shared_weight;
    }


    size_t ChaosGroup::EffectTree::reserve_slot(Tag::combo_id combo) {
        total_effect_count++;
        auto it = combo_subgroups.find(combo);
        if (it == combo_subgroups.end()) {
            auto [it_, res] = combo_subgroups.emplace(combo, *this);
            it = it_;
        }
        return it->second.reserve_slot();
    }

    void ChaosGroup::EffectTree::reset_size() {
        total_effect_count = 0;
        for (auto& [combo, subgroup] : combo_subgroups) {
            subgroup.reset_size();
        }
    }

    void ChaosGroup::EffectTree::alloc_nodes() {
        nodes = std::make_unique<Node[]>(size());

        for (auto& [combo, subgroup] : combo_subgroups) {
            subgroup.alloc_nodes();
        }
    }


    bool ChaosGroup::EffectTree::is_counted(Tag::combo_id combo) {
        return Tag::is_combo_allowed(combo);
    }

    void ChaosGroup::EffectTree::init_tree() {
        for (auto& [combo, subgroup] : combo_subgroups) {
            subgroup.init_tree();
        }

        size_t t_size = size();
        size_t combo_count = combo_subgroups.size();

        std::memcpy(nodes.get(), 0, (t_size - combo_count) * sizeof(Info));
        subgroup_tree_pos.clear();

        auto it = combo_subgroups.begin();

        for (size_t i = t_size - combo_count + 1; i <= t_size; i++) {
            Node& node = nodes[i - 1];
            auto& [combo, subtree] = *it;
            ++it;

            node.combo = combo;
            subgroup_tree_pos.emplace(combo, i - 1);

            size_t deviation = subtree.deviation_sum;
            size_t c = is_counted(combo) ? subtree.count : 0;

            bool last_child_left = (i % 2 == 0);

            for (size_t j = i / 2; j > 0; j /= 2) {
                Node& parent = nodes[j - 1];
                Info& info = parent.info;

                if (last_child_left) {
                    info.left_deviation_sum += deviation;
                    info.left_count += c;
                }
                last_child_left = (i % 2 == 0);
            }

            deviation_sum += deviation;
            count += c;
        }
    }

    void ChaosGroup::EffectTree::update_deviations_upwards(Tag::combo_id combo, f32 delta) {
        size_t i = subgroup_tree_pos.at(combo) + 1;

        bool last_child_left = (i % 2 == 0);
        for (int j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];
            Info& info = parent.info;

            if (last_child_left) {
                info.left_deviation_sum += delta;
            }
            last_child_left = (i % 2 == 0);
        }
    }

    void ChaosGroup::EffectTree::update_counts_upwards(Tag::combo_id combo, size_t delta) {
        size_t i = subgroup_tree_pos.at(combo) + 1;

        bool last_child_left = (i % 2 == 0);
        for (int j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];
            Info& info = parent.info;

            if (last_child_left) {
                info.left_count += delta;
            }
            last_child_left = (i % 2 == 0);
        }
    }

    void ChaosGroup::EffectTree::update_deviations_upwards(
        EffectSubtree::Node& start, EffectSubtree& subgroup, f32 delta) {
        subgroup.update_deviations_upwards(start, delta);
        if (subgroup.is_active) {
            update_deviations_upwards(start.effect.combo, delta);
        }
    }

    void ChaosGroup::EffectTree::update_counts_upwards(
        EffectSubtree::Node& start, EffectSubtree& subgroup, size_t delta) {
        subgroup.update_counts_upwards(start, delta);
        if (subgroup.is_active) {
            update_counts_upwards(start.effect.combo, delta);
        }
    }


    ChaosGroup::EffectSubtree& ChaosGroup::EffectTree::get_subgroup(Tag::combo_id combo) {
        return combo_subgroups.at(combo);
    }

    ChaosGroup::EffectSubtree& ChaosGroup::EffectTree::get_subgroup(f32 weight) {
        size_t t_size = size();
        size_t subgroups_count = combo_subgroups.size();

        size_t i;
        for (i = 1; i <= t_size - subgroups_count;) {
            Node& node = nodes[i - 1];
            if (get_left_weight(node) <= weight) {
                i = i * 2;
            } else {
                i = i * 2 + 1;
            }
        }
        Tag::combo_id combo = nodes[i - 1].combo;

        return combo_subgroups.at(combo);
    }

    void ChaosGroup::EffectTree::share_weight(
            EffectSubtree::Node& node, EffectSubtree& subgroup, f32 share_ratio) {
        f32 weight_share = get_weight(node) * share_ratio;
        f32 weight_share_per_effect = weight_share / (total_effect_count - 1);
        shared_weight += weight_share_per_effect;

        f32 delta = -(weight_share + weight_share_per_effect);
        node.weight_deviation += delta;

        update_deviations_upwards(node, subgroup, delta);
    }

    void ChaosGroup::EffectTree::activate_node(
            EffectSubtree::Node& node, EffectSubtree& subgroup) {
        if (!node.is_active) {
            update_deviations_upwards(node, subgroup, node.weight_deviation);
            update_counts_upwards(node, subgroup, 1);
            node.is_active = true;
        }
    }

    void ChaosGroup::EffectTree::deactivate_node(
            EffectSubtree::Node& node, EffectSubtree& subgroup) {
        if (node.is_active) {
            update_deviations_upwards(node, subgroup, -node.weight_deviation);
            update_counts_upwards(node, subgroup, -1);
            node.is_active = false;
        }
    }

    void ChaosGroup::EffectTree::activate_subgroup(Tag::combo_id combo) {
        auto it = combo_subgroups.find(combo);
        if (it != combo_subgroups.end()) {
            auto& [combo, subgroup] = *it;
            if (!subgroup.is_active) {
                update_deviations_upwards(combo, subgroup.deviation_sum);
                update_counts_upwards(combo, subgroup.count);
                subgroup.is_active = true;
            }
        }
    }

    void ChaosGroup::EffectTree::deactivate_subgroup(Tag::combo_id combo) {
        auto it = combo_subgroups.find(combo);
        if (it != combo_subgroups.end()) {
            auto& [combo, subgroup] = *it;
            if (subgroup.is_active) {
                update_deviations_upwards(combo, -subgroup.deviation_sum);
                update_counts_upwards(combo, -subgroup.count);
                subgroup.is_active = false;
            }
        }
    }



    ChaosGroup::ChaosGroup(const ChaosGroupSettings& settings) : settings(settings) {
        probability = settings.initial_probability;
    }


    f32 ChaosGroup::get_probability() const {
        return probability;
    }

    void ChaosGroup::apply_on_pick_multiplier() {
        probability *= settings.on_pick_multiplier;
    }


    u32 ChaosGroup::get_effect_count() const {
        return tree.count;
    }

    void ChaosGroup::reset_effect_count() {
        tree.reset_size();
    }

    size_t ChaosGroup::reserve_effect_slot(Tag::combo_id combo) {
        return tree.reserve_slot(combo);
    }

    void ChaosGroup::alloc_effect_slots() {
        tree.alloc_nodes();
    }


    // TODO rewrite
    ChaosEffectEntity& ChaosGroup::get_effect(Tag::combo_id combo, size_t pos) {
        return tree.combo_subgroups.at(combo).nodes[pos].effect;
    }

    f32 ChaosGroup::get_effect_weight(ChaosEffectEntity& effect) {
        EffectSubtree::Node& node = reinterpret_cast<EffectSubtree::Node&>(effect);
        return tree.get_weight(node);
    }


    void ChaosGroup::init_tree() {
        tree.init_tree();
    }

    f32 ChaosGroup::get_weight_sum() {
        return tree.get_weight_sum();
    }

    ChaosEffectEntity& ChaosGroup::get_effect_entity_by_weight(f32 weight) {
        return tree.get_subgroup(weight).get_node(weight).effect;
    }


    ChaosEffectEntity& ChaosGroup::pick_effect() {
        u32 effect_count = get_effect_count();
        f32 rand = Rand_ZeroOne() * get_weight_sum();

        EffectSubtree& subgroup = tree.get_subgroup(rand);
        EffectSubtree::Node& node = subgroup.get_node(rand);

        if (effect_count > 1) {
            tree.share_weight(node, subgroup, settings.winner_weight_share);
        }

        return node.effect;
    }

    void ChaosGroup::set_effect_status(ChaosEffectEntity& effect, ChaosEffectStatus status) {
        if (effect.status == status) {
            return;
        }

        if (status == ChaosEffectStatus::ACTIVE) {
            auto affected = Tag::reserve_combo(effect.combo);
            deactivate_subgroups(affected);
        } else if (effect.status == ChaosEffectStatus::ACTIVE) {
            auto affected = Tag::free_combo(effect.combo);
            activate_subgroups(affected);
        }

        EffectSubtree::Node& node = reinterpret_cast<EffectSubtree::Node&>(effect);
        EffectSubtree& subgroup = tree.get_subgroup(effect.combo);

        switch(status) {
            case ChaosEffectStatus::AVAILABLE: {
                tree.activate_node(node, subgroup);
            }
            case ChaosEffectStatus::ACTIVE:
            case ChaosEffectStatus::HIDDEN:
            case ChaosEffectStatus::DISABLED: {
                tree.deactivate_node(node, subgroup);
            }
        }
    }

    void ChaosGroup::activate_subgroup(Tag::combo_id combo) {
        tree.activate_subgroup(combo);
    }

    void ChaosGroup::deactivate_subgroup(Tag::combo_id combo) {
        tree.deactivate_subgroup(combo);
    }
}
