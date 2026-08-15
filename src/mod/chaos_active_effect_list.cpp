#include "chaos.h"

namespace Chaos {
    extern GameCtx* _ctx;

    static inline void effect_start(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.on_start_fun != nullptr) {
            effect.on_start_fun(ctx);
        }

        debug_log("Effect '%s' started.", effect.name);
    }

    static inline void effect_update(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.update_fun != nullptr) {
            effect.update_fun(ctx);
        }
    }

    static inline void effect_end(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.on_end_fun != nullptr) {
            effect.on_end_fun(ctx);
        }

        debug_log("Effect '%s' ended.", effect.name);
    }

    static inline void effect_pause(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.on_pause_fun != nullptr) {
            queue_pause_fun(&effect);
        }

        debug_log("Effect '%s' paused.", effect.name);
    }

    static inline void effect_unpause(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.on_unpause_fun != nullptr) {
            queue_unpause_fun(&effect);
        }

        debug_log("Effect '%s' unpaused.", effect.name);
    }

    void ActiveChaosEffectList::queue_for_remove_entity(ChaosEffectEntity& entity) {
        Node* prev = nullptr;

        for (Node* cur = root.get(); cur != nullptr; cur = cur->next.get()) {
            if (cur->effect == &entity) {
                move_node(root, remove_root, prev);
                break;
            }

            prev = cur;
        }
    }

    void ActiveChaosEffectList::add(ChaosGroup& group, ChaosEffectEntity& entity) {
        std::unique_ptr<Node> n = std::make_unique<Node>();

        n->effect = &entity;
        n->group = &group;
        n->timer = 0;
        n->next = std::move(root);
        root = std::move(n);

        if (entity.status == ChaosEffectStatus::AVAILABLE) {
            group.set_effect_status(entity, ChaosEffectStatus::ACTIVE);
        }

        ChaosEffect& effect = entity.effect;
        effect_start(effect, _ctx);
    }


    void ActiveChaosEffectList::update() {
        Node* prev = nullptr;

        for (Node* cur = root.get(); cur != nullptr; cur = cur->next.get()) {
            ChaosEffectEntity& entity = *cur->effect;
            ChaosEffect& effect = entity.effect;

            effect_update(effect, _ctx);

            if (cur->timer >= effect.duration) {
                remove_after(prev);
                continue;
            }
            cur->timer++;

            prev = cur;
        }
    }

    void ActiveChaosEffectList::empty_remove_queue() {
        std::unique_ptr<Node> cur = std::move(remove_root);
        while (cur.get() != nullptr) {
            ChaosEffectEntity& entity = *cur->effect;
            ChaosEffect& effect = entity.effect;

            effect_update(effect, _ctx);
            effect_end(effect, _ctx);

            std::unique_ptr<Node> tmp = std::move(cur);
            cur = std::move(tmp->next);
        }

        remove_root = nullptr;
    }

    void ActiveChaosEffectList::pause_effects(const std::unordered_set<Tag::combo_id>& affected_combos) {
        Node* prev_start = pause_root.get();

        move_nodes(root, pause_root, affected_combos);

        for (Node* cur = pause_root.get(); cur != prev_start; cur = cur->next.get()) {
            ChaosEffectEntity& entity = *cur->effect;
            ChaosEffect& effect = entity.effect;
            effect_pause(effect, _ctx);
        }
    }

    void ActiveChaosEffectList::unpause_effects(const std::unordered_set<Tag::combo_id>& affected_combos) {
        Node* prev_start = root.get();

        move_nodes(pause_root, root, affected_combos);

        for (Node* cur = root.get(); cur != prev_start; cur = cur->next.get()) {
            ChaosEffectEntity& entity = *cur->effect;
            ChaosEffect& effect = entity.effect;
            effect_unpause(effect, _ctx);
        }
    }


    void ActiveChaosEffectList::move_node(
            std::unique_ptr<Node>& from_root, std::unique_ptr<Node>& to_root, Node* element) {

        std::unique_ptr<Node> moved;
        if (element == nullptr) {
            moved = std::move(from_root);
            from_root = std::move(moved->next);
        } else {
            moved = std::move(element->next);
            element->next = std::move(moved->next);
        }

        moved->next = std::move(to_root);
        to_root = std::move(moved);
    }

    void ActiveChaosEffectList::move_nodes(
            std::unique_ptr<Node>& from_root,
            std::unique_ptr<Node>& to_root,
            const std::unordered_set<Tag::combo_id>& affected_combos) {

        Node* prev = nullptr;

        for (Node* cur = from_root.get(); cur != nullptr; cur = cur->next.get()) {
            Tag::combo_id combo = cur->effect->combo;
            if (affected_combos.contains(combo)) {
                move_node(from_root, to_root, prev);
            }

            prev = cur;
        }
    }

    void ActiveChaosEffectList::remove_after(Node* element) {
        std::unique_ptr<Node> del;

        if (element == nullptr) {
            del = std::move(root);
            root = std::move(del->next);
        } else {
            del = std::move(element->next);
            element->next = std::move(del->next);
        }

        ChaosEffectEntity& entity = *del->effect;
        ChaosEffect& effect = entity.effect;

        if (entity.status == ChaosEffectStatus::ACTIVE) {
            ChaosGroup& group = *del->group;
            group.set_effect_status(entity, ChaosEffectStatus::AVAILABLE);
        }

        effect_end(effect, _ctx);
    }
}