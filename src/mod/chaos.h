#ifndef __CHAOS_H__
#define __CHAOS_H__

#include "util/debug.h"

#include "modding.h"
#include "recomputils.h"

#ifdef __cplusplus
#include "math.h"

#define this this_
extern "C" {
#include "global.h"
}
#undef this

namespace Chaos {
extern "C" {
#else
#include "global.h"
#endif

typedef PlayState GameCtx;

void chaos_init(void);
void chaos_update(GameCtx* play);

extern bool chaos_is_player_active;

#ifdef __cplusplus
} /* extern "C" */
} /* namespace Chaos */

#include "tag.h"

#include "util/static_vector.h"
#include "util/finite_vector.h"

#include <memory>
#include <unordered_map>
#include <utility>

namespace Chaos {
    typedef void (*ChaosFunction)(GameCtx* play);

    enum Disturbance : int {
        VERY_LOW,
        LOW,
        MEDIUM,
        HIGH,
        VERY_HIGH,
        NIGHTMARE,
        MAX,
    };

    typedef struct {
        const char* name;
        u32 duration; // In frames.

        ChaosFunction on_start_fun;
        ChaosFunction update_fun;
        ChaosFunction on_end_fun;
    } ChaosEffect;


    enum ChaosEffectStatus {
        AVAILABLE,
        ACTIVE,
        HIDDEN,
        DISABLED,
    };

    class ChaosGroup;

    typedef struct {
        ChaosEffect effect;
        ChaosEffectStatus status;
        ChaosGroup* owner;
        Tag::combo_id combo;
    } ChaosEffectEntity;


    typedef struct {
        f32 initial_probability;
        f32 on_pick_multiplier;
        f32 winner_weight_share;
    } ChaosGroupSettings;

    typedef struct {
        const char* name;
        u32 cycle_length;
        ChaosGroupSettings default_groups_settings[Disturbance::MAX];
    } ChaosMachineSettings;


    class ChaosGroup {
    private:
        struct EffectTree;

        struct EffectSubtree {
            struct Node {
                ChaosEffectEntity effect;
                f32 weight_deviation = 0.0;
                f32 left_deviation_sum = 0.0; // sum of weight deviations of active
                                        // weights in left subtree.
                size_t left_count = 0;      // number of active nodes in left subtree.
                bool is_active = true;
            };

            EffectTree& owner;
            bool is_active = true;

            size_t size_ = 0;
            std::unique_ptr<Node[]> nodes;
            size_t count = 0;
            f32 deviation_sum = 0;

            EffectSubtree(EffectTree& owner) : owner(owner) {};

            // size_t size();
            f32 get_weight(Node& node);
            f32 get_left_weight(Node& node);

            size_t reserve_slot();
            void reset_size();
            void alloc_nodes();

            bool is_counted(Node& node);
            f32 get_deviation(Node& node);
            void init_tree();
            void update_deviations_upwards(Node& start, f32 delta);
            void update_counts_upwards(Node& start, size_t delta);

            Node& get_node(f32 weight);
            size_t get_pos(Node& node);
        };

        struct EffectTree {
            struct Info {
                f32 left_deviation_sum; // sum of weight deviations of active
                                        // weights in left subtree.
                size_t left_count;      // number of active nodes in left subtree.
            };

            union Node {
                Info info;
                Tag::combo_id combo;
            };

            std::unique_ptr<Node[]> nodes;
            size_t count = 0;
            f32 deviation_sum = 0;
            f32 shared_weight = 1.0f; // per effect
            std::unordered_map<Tag::combo_id, EffectSubtree> combo_subgroups;
            std::unordered_map<Tag::combo_id, size_t> subgroup_tree_pos;

            size_t total_effect_count;

            size_t size();
            f32 get_weight(EffectSubtree::Node& node);
            f32 get_weight_sum();
            f32 get_left_weight(Node& node);

            size_t reserve_slot(Tag::combo_id combo);
            void reset_size();
            void alloc_nodes();

            bool is_counted(Tag::combo_id combo);
            void init_tree();
            void update_deviations_upwards(Tag::combo_id combo, f32 delta);
            void update_counts_upwards(Tag::combo_id combo, size_t delta);
            void update_deviations_upwards(
                EffectSubtree::Node& start, EffectSubtree& subgroup, f32 delta);
            void update_counts_upwards(
                EffectSubtree::Node& start, EffectSubtree& subgroup, size_t delta);

            ChaosGroup::EffectSubtree& get_subgroup(Tag::combo_id combo);
            ChaosGroup::EffectSubtree& get_subgroup(f32 weight);
            void share_weight(EffectSubtree::Node& node, EffectSubtree& subgroup, f32 share);
            void activate_node(EffectSubtree::Node& node, EffectSubtree& subgroup);
            void deactivate_node(EffectSubtree::Node& node, EffectSubtree& subgroup);
            void activate_subgroup(Tag::combo_id combo);
            void deactivate_subgroup(Tag::combo_id combo);
        };

        ChaosGroupSettings settings;
        f32 probability;

        EffectTree tree;

    public:
        ChaosGroup(const ChaosGroupSettings& settings);

        f32 get_probability() const;
        void apply_on_pick_multiplier();

        u32 get_effect_count() const;
        void reset_effect_count();
        size_t reserve_effect_slot(Tag::combo_id combo);
        void alloc_effect_slots();

        ChaosEffectEntity& get_effect(Tag::combo_id combo, size_t pos);
        f32 get_effect_weight(ChaosEffectEntity& effect);

        void init_tree();
        f32 get_weight_sum();
        ChaosEffectEntity& get_effect_entity_by_weight(f32 weight);
        ChaosEffectEntity& get_effect_entity_by_weight(EffectSubtree& tree, f32 weight);

        ChaosEffectEntity& pick_effect();
        void set_effect_status(ChaosEffectEntity& effect, ChaosEffectStatus status);
        void activate_subgroup(Tag::combo_id combo);
        void deactivate_subgroup(Tag::combo_id combo);

    private:
        // size_t root_tree_size() const;
        u32 get_effect_entity_pos(ChaosEffectEntity& entity);
    };

    class ActiveChaosEffectList {
    private:
        struct Node {
            ChaosEffectEntity* effect;
            ChaosGroup* group;
            u32 timer;
            std::unique_ptr<Node> next;
        };

        std::unique_ptr<Node> root;
        std::unique_ptr<Node> remove_root;

    public:
        void queue_for_remove_entity(ChaosEffectEntity& entity);
        void add(ChaosGroup& group, ChaosEffectEntity& entity);

        void update();
        void empty_remove_queue();

    private:
        void queue_for_remove_after(Node* element);
        void remove_after(ChaosGroup& group, Node* element);
    };

    class ChaosMachine {
    private:
        ChaosMachineSettings settings;
        static_vector<ChaosGroup, Disturbance::MAX> groups;
        u32 cycle_timer = 0;
        ActiveChaosEffectList active_effects;
        u8 roll_requests = 0;
        u8 group_roll_requests[Disturbance::MAX];

    public:
        ChaosMachine(const ChaosMachineSettings& settings);

        ChaosMachineSettings& get_settings();
        Disturbance get_group_disturbance(ChaosGroup* group) const;
        ChaosGroup& get_group(Disturbance disturbance);
        ChaosGroup* pick_group();

        void perform_roll(ChaosGroup& group);
        void perform_roll(Disturbance disturbance);
        void perform_roll();

        void update();

        void enable_effect(ChaosEffectEntity& entity);
        void disable_effect(ChaosEffectEntity& entity);
        void stop_effect(ChaosEffectEntity& entity);
    };


    ChaosMachine& get_machine(size_t pos);
    ChaosMachine& get_machine(ChaosGroup& group);
    ChaosMachine* get_machine_or_null(size_t pos) noexcept;

    ChaosMachine* register_machine(const ChaosMachineSettings& settings);
    ChaosEffectEntity* register_effect(
        ChaosMachine* machine, const ChaosEffect& effect, Disturbance disturbance,
        const char** tagnames, size_t tagcount);

    void init();

    void enable_effect(ChaosEffectEntity& entity);
    void disable_effect(ChaosEffectEntity& entity);
    void stop_effect(ChaosEffectEntity& entity);

    void request_roll(ChaosMachine& machine);
    void request_roll(ChaosMachine& machine, Disturbance disturbance);

    size_t get_machine_count();

    void activate_subgroups(std::unordered_set<Tag::combo_id> subgroups);
    void deactivate_subgroups(std::unordered_set<Tag::combo_id> subgroups);

    extern const char* DISTURBANCE_NAME[];
    extern bool debug_disable_rolling;
}

#endif

#endif /* __CHAOS_H__ */
