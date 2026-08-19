#include "chaos.h"

#include <memory>
#include <cstring>

namespace Chaos {
    double ChaosGroup::EffectSubtree::get_weight(Node& node) const {
        return node.weight_deviation + owner.shared_weight;
    }

    double ChaosGroup::EffectSubtree::get_left_weight(Node& node) const {
        return node.left_deviation_sum + node.left_count * owner.shared_weight;
    }


    size_t ChaosGroup::EffectSubtree::reserve_slot() {
        return _size++;
    }

    void ChaosGroup::EffectSubtree::reset_size() {
        _size = 0;
    }

    void ChaosGroup::EffectSubtree::alloc_nodes() {
        nodes = std::make_unique<Node[]>(_size);
    }


    bool ChaosGroup::EffectSubtree::is_counted(Node& node) const {
        return (node.effect.status == ChaosEffectStatus::AVAILABLE);
    }

    double ChaosGroup::EffectSubtree::get_deviation(Node& node) const {
        if (is_counted(node)) {
            return node.weight_deviation;
        }
        return 0.0;
    }

    void ChaosGroup::EffectSubtree::init_tree() {
        for (size_t i = _size; i > 0; i--) {
            Node& node = nodes[i - 1];

            double deviation = get_deviation(node);
            size_t c = is_counted(node) ? 1 : 0;

            bool last_child_left = (i % 2 == 0);

            for (size_t j = i / 2; j > 0; j /= 2) {
                Node& parent = nodes[j - 1];

                if (last_child_left) {
                    parent.left_deviation_sum += deviation;
                    parent.left_count += c;
                }
                last_child_left = (j % 2 == 0);
            }

            deviation_sum += deviation;
            count += c;
        }
    }

    void ChaosGroup::EffectSubtree::update_deviations_upwards(Node& node, double delta) {
        size_t i = get_pos(node) + 1;

        bool last_child_left = (i % 2 == 0);
        for (size_t j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];

            if (last_child_left) {
                parent.left_deviation_sum += delta;
            }
            last_child_left = (j % 2 == 0);
        }

        deviation_sum += delta;
    }

    void ChaosGroup::EffectSubtree::update_count(Node& node, size_t delta) {
        size_t i = get_pos(node) + 1;

        bool last_child_left = (i % 2 == 0);
        for (size_t j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];

            if (last_child_left) {
                parent.left_count += delta;
            }
            last_child_left = (j % 2 == 0);
        }

        count += delta;
    }


    ChaosGroup::EffectSubtree::Node& ChaosGroup::EffectSubtree::get_node(double weight) {
        for (size_t i = 1; i <= _size;) {
            Node& node = nodes[i - 1];
            if (get_left_weight(node) >= weight) {
                i = i * 2;
            } else if (get_left_weight(node) + get_weight(node) >= weight) {
                return node;
            } else {
                i = i * 2 + 1;
                weight -= get_left_weight(node) + get_weight(node);
            }
        }

        // // Shouldn't happen
        // error("get_effect_entity_by_weight didn't find any node within given weight limit. "
        //       "Returning first node.");
        return nodes[0];
    }

    size_t ChaosGroup::EffectSubtree::get_pos(Node& node) {
        return &node - nodes.get();
    }



    size_t ChaosGroup::EffectTree::size() {
        if (subgroups.size() == 0) {
            return 0;
        }
        return subgroups.size() * 2 - 1;
    }

    double ChaosGroup::EffectTree::get_weight(EffectSubtree::Node& node) const {
        return node.weight_deviation + shared_weight;
    }

    double ChaosGroup::EffectTree::get_weight_sum() const {
        return deviation_sum + count * shared_weight;
    }

    double ChaosGroup::EffectTree::get_left_weight(Node& node) const {
        Info& info = node.info;
        return info.left_deviation_sum + info.left_count * shared_weight;
    }


    size_t ChaosGroup::EffectTree::reserve_slot(Tag::combo_id combo) {
        total_effect_count++;
        auto it = subgroups.find(combo);
        if (it == subgroups.end()) {
            auto [it_, res] = subgroups.emplace(combo, *this);
            it = it_;
        }
        return it->second.subtree.reserve_slot();
    }

    void ChaosGroup::EffectTree::reset_size() {
        total_effect_count = 0;
        for (auto& [combo, subgroup_data] : subgroups) {
            subgroup_data.subtree.reset_size();
        }
    }

    void ChaosGroup::EffectTree::alloc_nodes() {
        nodes = std::make_unique<Node[]>(size());

        for (auto& [combo, subgroup_data] : subgroups) {
            subgroup_data.subtree.alloc_nodes();
        }
    }


    bool ChaosGroup::EffectTree::is_counted(Tag::combo_id combo) const {
        return Tag::is_combo_allowed(combo);
    }

    void ChaosGroup::EffectTree::init_tree() {
        for (auto& [combo, subgroup_data] : subgroups) {
            subgroup_data.subtree.init_tree();
        }

        size_t t_size = size();
        size_t combo_count = subgroups.size();

        std::memset(nodes.get(), 0, (t_size - combo_count) * sizeof(Info));

        auto it = subgroups.begin();

        for (size_t i = t_size - combo_count + 1; i <= t_size; i++) {
            Node& node = nodes[i - 1];
            auto& [combo, subgroup_data] = *it;
            ++it;
            EffectSubtree& subtree = subgroup_data.subtree;

            node.combo = combo;
            subgroup_data.node_pos = i;

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
                last_child_left = (j % 2 == 0);
            }

            deviation_sum += deviation;
            count += c;
        }
    }

    void ChaosGroup::EffectTree::update_deviations_upwards(Tag::combo_id combo, double delta) {
        size_t i = subgroups.at(combo).node_pos;

        bool last_child_left = (i % 2 == 0);
        for (int j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];
            Info& info = parent.info;

            if (last_child_left) {
                info.left_deviation_sum += delta;
            }
            last_child_left = (j % 2 == 0);
        }

        deviation_sum += delta;
    }

    void ChaosGroup::EffectTree::update_count(Tag::combo_id combo, size_t delta) {
        size_t i = subgroups.at(combo).node_pos;

        bool last_child_left = (i % 2 == 0);
        for (int j = i / 2; j > 0; j /= 2) {
            Node& parent = nodes[j - 1];
            Info& info = parent.info;

            if (last_child_left) {
                info.left_count += delta;
            }
            last_child_left = (j % 2 == 0);
        }

        count += delta;
    }

    void ChaosGroup::EffectTree::update_deviations_upwards(
            EffectSubtree::Node& node, EffectSubtree& subgroup, double delta) {
        subgroup.update_deviations_upwards(node, delta);
        if (subgroup.is_active) {
            update_deviations_upwards(node.effect.combo, delta);
        }
    }

    void ChaosGroup::EffectTree::update_counts_upwards(
            EffectSubtree::Node& node, EffectSubtree& subgroup, size_t delta) {
        subgroup.update_count(node, delta);
        if (subgroup.is_active) {
            update_count(node.effect.combo, delta);
        }
    }


    ChaosGroup::EffectSubtree& ChaosGroup::EffectTree::get_subgroup(Tag::combo_id combo) {
        return subgroups.at(combo).subtree;
    }

    ChaosGroup::EffectSubtree& ChaosGroup::EffectTree::get_subgroup(double weight, double* local_weight_out) {
        size_t t_size = size();
        size_t subgroups_count = subgroups.size();

        double local_weight = weight;
        size_t i;
        for (i = 1; i <= t_size - subgroups_count;) {
            Node& node = nodes[i - 1];
            double left_weight = get_left_weight(node);
            if (left_weight <= weight) {
                i = i * 2;
            } else {
                i = i * 2 + 1;
                local_weight -= left_weight;
            }
        }
        Tag::combo_id combo = nodes[i - 1].combo;

        if (local_weight_out) {
            *local_weight_out = local_weight;
        }

        return subgroups.at(combo).subtree;
    }

    void ChaosGroup::EffectTree::share_weight(
            EffectSubtree::Node& node, EffectSubtree& subgroup, double share_ratio) {
        double weight_share_total = get_weight(node) * share_ratio;
        double weight_share_per_effect = weight_share_total / (total_effect_count - 1);
        shared_weight += weight_share_per_effect;

        double delta = -(weight_share_total + weight_share_per_effect);

        node.weight_deviation += delta;
        update_deviations_upwards(node, subgroup, delta);

        if (shared_weight > total_effect_count) {
            normalize_weight_share();
        }
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
        auto it = subgroups.find(combo);
        if (it != subgroups.end()) {
            auto& [combo, subgroup] = *it;
            EffectSubtree& subtree = subgroup.subtree;
            if (!subtree.is_active) {
                update_deviations_upwards(combo, subtree.deviation_sum);
                update_count(combo, subtree.count);
                subtree.is_active = true;
            }
        }
    }

    void ChaosGroup::EffectTree::deactivate_subgroup(Tag::combo_id combo) {
        auto it = subgroups.find(combo);
        if (it != subgroups.end()) {
            auto& [combo, subgroup] = *it;
            EffectSubtree& subtree = subgroup.subtree;
            if (subtree.is_active) {
                update_deviations_upwards(combo, -subtree.deviation_sum);
                update_count(combo, -subtree.count);
                subtree.is_active = false;
            }
        }
    }

    void ChaosGroup::EffectTree::normalize_weight_share() {
        double total_sum = deviation_sum + total_effect_count * shared_weight;
        double scale = total_effect_count / total_sum;

        double delta = shared_weight - 1.0 / scale;

        for (auto it = subgroups.begin(); it != subgroups.end(); ++it) {
            auto& [combo, subgroup] = *it;
            EffectSubtree& subtree = subgroup.subtree;

            for (size_t i = 0; i < subtree._size; i++) {
                EffectSubtree::Node& node = subtree.nodes[i];

                double prev_deviation = node.weight_deviation;
                node.weight_deviation += delta;
                node.weight_deviation *= scale;

                if (node.is_active) {
                    double change = node.weight_deviation - prev_deviation;
                    subtree.update_deviations_upwards(node, change);
                }
            }

            if (subtree.is_active) {
                update_deviations_upwards(combo, delta * subtree.count);
            }
        }

        shared_weight = 1.0;
    }


    ChaosEffectEntity& ChaosGroup::EffectIterator::operator*() const {
        EffectSubtree& subtree = tree_it->second.subtree;
        return subtree.nodes[subtree_pos].effect;
    }

    ChaosGroup::EffectIterator& ChaosGroup::EffectIterator::operator++() {
        subtree_pos++;
        if (subtree_pos >= tree_it->second.subtree._size) {
            subtree_pos = 0;
            ++tree_it;
        }
        return *this;
    }

    bool ChaosGroup::EffectIterator::operator==(const EffectIterator& other) const {
        return ((tree_it == other.tree_it) && (subtree_pos == other.subtree_pos));
    }


    ChaosGroup::ChaosGroup(const ChaosGroupSettings& settings) : settings(settings) {
        probability = settings.initial_probability;
    }


    double ChaosGroup::get_probability() const {
        return probability;
    }

    void ChaosGroup::apply_on_pick_multiplier() {
        probability *= settings.on_pick_multiplier;
    }


    ChaosGroup::EffectIterator ChaosGroup::begin() {
        return EffectIterator(tree.subgroups.begin(), 0);
    }

    ChaosGroup::EffectIterator ChaosGroup::end() {
        return EffectIterator(tree.subgroups.end(), 0);
    }


    size_t ChaosGroup::size() const {
        return tree.total_effect_count;
    }

    size_t ChaosGroup::get_effect_count() const {
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
        return tree.subgroups.at(combo).subtree.nodes[pos].effect;
    }

    double ChaosGroup::get_effect_weight(ChaosEffectEntity& effect) {
        EffectSubtree::Node& node = reinterpret_cast<EffectSubtree::Node&>(effect);
        return tree.get_weight(node);
    }


    void ChaosGroup::init_tree() {
        tree.init_tree();
    }

    double ChaosGroup::get_weight_sum() const {
        return tree.get_weight_sum();
    }

    ChaosEffectEntity& ChaosGroup::get_effect_entity_by_weight(double weight) {
        return tree.get_subgroup(weight).get_node(weight).effect;
    }


    ChaosEffectEntity& ChaosGroup::pick_effect(double rand) {
        if ((rand < 0) || (rand > 1)) {
            rand = Rand_ZeroOne();
        }
        double weight = rand * get_weight_sum();
        return pick_effect_by_weight(weight);
    }

    ChaosEffectEntity& ChaosGroup::pick_effect_by_weight(double weight) {
        double local_weight;
        EffectSubtree& subgroup = tree.get_subgroup(weight, &local_weight);
        EffectSubtree::Node& node = subgroup.get_node(local_weight);

        u32 effect_count = get_effect_count();
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
                break;
            }
            case ChaosEffectStatus::ACTIVE:
            case ChaosEffectStatus::HIDDEN:
            case ChaosEffectStatus::DISABLED: {
                tree.deactivate_node(node, subgroup);
                break;
            }
        }

        effect.status = status;
    }

    void ChaosGroup::activate_subgroup(Tag::combo_id combo) {
        tree.activate_subgroup(combo);
    }

    void ChaosGroup::deactivate_subgroup(Tag::combo_id combo) {
        tree.deactivate_subgroup(combo);
    }
}
