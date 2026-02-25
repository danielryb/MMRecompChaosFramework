#include "chaos.h"

namespace Chaos {
    extern GameCtx* _ctx;

    static inline void effect_start(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.on_start_fun != NULL) {
            effect.on_start_fun(ctx);
        }

        debug_log("Effect '%s' started.", effect.name);
    }

    static inline void effect_update(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.update_fun != NULL) {
            effect.update_fun(ctx);
        }
    }

    static inline void effect_end(ChaosEffect& effect, GameCtx* ctx) {
        if (effect.on_end_fun != NULL) {
            effect.on_end_fun(ctx);
        }

        debug_log("Effect '%s' ended.", effect.name);
    }

    void ActiveChaosEffectList::queue_for_remove_entity(ChaosEffectEntity& entity) {
        Node* prev = nullptr;

        for (Node* cur = root.get(); cur != nullptr; cur = cur->next.get()) {
            if (cur->effect == &entity) {
                queue_for_remove_after(prev);
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

        for (Node* cur = root.get(); cur != NULL; cur = cur->next.get()) {
            ChaosEffectEntity& entity = *cur->effect;
            ChaosEffect& effect = entity.effect;

            effect_update(effect, _ctx);

            if (cur->timer >= effect.duration) {
                ChaosGroup& group = *cur->group;
                remove_after(group, prev);
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

    void ActiveChaosEffectList::queue_for_remove_after(Node* element) {
        std::unique_ptr<Node> removed;
        if (element == NULL) {
            removed = std::move(root);
            root = std::move(removed->next);
        } else {
            removed = std::move(element->next);
            element->next = std::move(removed->next);
        }

        removed->next = std::move(remove_root);
        remove_root = std::move(removed);
    }

    void ActiveChaosEffectList::remove_after(ChaosGroup& group, Node* element) {
        std::unique_ptr<Node> del;

        if (element == NULL) {
            del = std::move(root);
            root = std::move(del->next);
        } else {
            del = std::move(element->next);
            element->next = std::move(del->next);
        }

        ChaosEffectEntity& entity = *del->effect;
        ChaosEffect& effect = entity.effect;

        if (entity.status == ChaosEffectStatus::ACTIVE) {
            group.set_effect_status(entity, ChaosEffectStatus::AVAILABLE);
        }

        effect_end(effect, _ctx);
    }
}