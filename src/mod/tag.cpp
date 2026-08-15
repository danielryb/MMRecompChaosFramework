#include "util/debug.h"
#include "tag.h"

#include <unordered_map>
#include <map>
#include <algorithm>

namespace Chaos {
    namespace Tag {
        using tag_id = int;
        using combo_id = int;

        constexpr tag_id FIRST_TAG_ID = 0;
        constexpr combo_id FIRST_COMBO_ID = 1;

        struct Tag {
            std::vector<combo_id> related_combos; // ids of combos containing this tag.
            size_t reservations = 1;
            bool excluded = false;
        };

        struct Combo {
            std::vector<tag_id> expanded; // expanded combo in the form of vector<tag>.
            size_t conflicts = 0;
            size_t exclusions = 0;
        };

        tag_id next_tag_id = FIRST_TAG_ID;
        combo_id next_combo_id = FIRST_COMBO_ID;

        std::unordered_map<std::string, tag_id> tags;
        std::map<std::vector<tag_id>, combo_id> combos;

        std::vector<Tag> tag_data;
        std::vector<Combo> combo_data;

        void clear() {
            next_tag_id = FIRST_TAG_ID;
            next_combo_id = FIRST_COMBO_ID;

            tags.clear();
            combos.clear();

            tag_data.clear();
            combo_data.clear();
        }


        inline Tag& get_tag_data(tag_id id) {
            return tag_data[id - FIRST_TAG_ID];
        }

        inline Combo& get_combo_data(combo_id id) {
            return combo_data[id - FIRST_COMBO_ID];
        }

        template<typename K, typename V, typename M>
        V get_id(const K& key, V& next_id_counter, M& id_map) {
            auto it = id_map.find(key);
            if (it != id_map.end()) {
                return it->second;
            } else {
                tag_id id = next_id_counter;
                next_id_counter++;

                id_map.emplace(key, id);
                return id;
            }
        }

        tag_id get_tag_id(const std::string& tagname) {
            tag_id prev_next_id = next_tag_id;
            tag_id id = get_id(tagname, next_tag_id, tags);

            if (prev_next_id != next_tag_id) {
                tag_data.emplace_back();
            }
            return id;
        }

        bool add_tag(const std::string& tagname, size_t reservation_limit) {
            tag_id prev_next_id = next_tag_id;
            tag_id id = get_tag_id(tagname);

            if (prev_next_id != next_tag_id) {
                Tag& tag = get_tag_data(id);
                tag.reservations = reservation_limit;
                return true;
            }
            return false;
        }

        combo_id get_combo_id(std::vector<tag_id>&& combo) {
            combo_id prev_next_id = next_combo_id;
            combo_id id = get_id(combo, next_combo_id, combos);

            if (prev_next_id != next_combo_id) {
                for (auto tag : combo) {
                    get_tag_data(tag).related_combos.push_back(id);
                }
                combo_data.emplace_back(std::move(combo));
            }
            return id;
        }

        combo_id get_combo_id(const std::vector<std::string>& tag_names) {
            std::vector<tag_id> combo;

            for (const std::string& tagname : tag_names) {
                combo.push_back(get_tag_id(tagname));
            }
            std::sort(combo.begin(), combo.end());

            return get_combo_id(std::move(combo));
        }

        combo_id get_combo_id(const char* tag_names[], size_t tag_count) {
            if (tag_count == 0) {
                return 0;
            }

            std::vector<std::string> vec;
            for (size_t i = 0; i < tag_count; i++) {
                const char* tagname = tag_names[i];
                vec.emplace_back(tagname);
            }
            return get_combo_id(vec);
        }


        template <int V>
        std::unordered_set<combo_id> modify_reservations(std::vector<tag_id> tags) {
            std::unordered_set<combo_id> affected_combos;

            std::unordered_set<combo_id> prev_allowed;
            for (auto tag : tags) {
                Tag& tag_data = get_tag_data(tag);
                auto& related = tag_data.related_combos;
                for (auto combo : related) {
                    if (is_combo_allowed(combo)) {
                        prev_allowed.insert(combo);
                    }
                }
            }

            for (auto tag : tags) {
                Tag& tag_data = get_tag_data(tag);

                tag_data.reservations += V;

                bool modify_conflicts = false;
                if constexpr (V < 0) {
                    if (tag_data.reservations == 0) {
                        modify_conflicts = true;
                    }
                } else {
                    if (tag_data.reservations == V) {
                        modify_conflicts = true;
                    }
                }

                if (modify_conflicts) {
                    constexpr int val_change = (V < 0) ? +1 : -1;

                    auto& related = tag_data.related_combos;
                    for (auto combo : related) {
                        Combo& combo_data = get_combo_data(combo);
                        combo_data.conflicts += val_change;
                    }
                }
            }

            for (auto tag : tags) {
                Tag& tag_data = get_tag_data(tag);
                auto& related = get_tag_data(tag).related_combos;
                for (auto combo : related) {
                    bool cur = is_combo_allowed(combo);
                    bool prev = prev_allowed.contains(combo);
                    if (cur != prev) {
                        affected_combos.insert(combo);
                    }
                }
            }

            return affected_combos;
        }

        template <int V>
        std::unordered_set<combo_id> modify_reservations(combo_id id) {
            if (id == 0) {
                std::unordered_set<combo_id> empty;
                return empty;
            }

            Combo& combo = get_combo_data(id);
            auto& expanded_combo = combo.expanded;

            return modify_reservations<V>(expanded_combo);
        }

        std::unordered_set<combo_id> reserve_combo(combo_id id) {
            return modify_reservations<-1>(id);
        }

        std::unordered_set<combo_id> free_combo(combo_id id) {
            return modify_reservations<+1>(id);
        }


        template <bool V>
        std::pair<bool, std::unordered_set<combo_id>> modify_tag_exclusion(tag_id id) {
            Tag& tag = get_tag_data(id);

            bool modified = (tag.excluded != V);
            std::unordered_set<combo_id> affected_combos;

            if (modified) {
                auto& related = tag.related_combos;

                std::unordered_set<combo_id> prev_allowed;
                for (auto combo : related) {
                    if (is_combo_allowed(combo)) {
                        prev_allowed.insert(combo);
                    }
                }

                tag.excluded = V;
                for (auto combo : related) {
                    constexpr int val_change = V ? +1 : -1;
                    get_combo_data(combo).exclusions += val_change;
                }

                for (auto combo : related) {
                    bool cur = is_combo_allowed(combo);
                    bool prev = prev_allowed.contains(combo);
                    if (cur != prev) {
                        affected_combos.insert(combo);
                    }
                }
            }
            return std::make_pair(modified, affected_combos);
        }

        std::pair<bool, std::unordered_set<combo_id>> include_tag(tag_id id) {
            return modify_tag_exclusion<false>(id);
        }

        std::pair<bool, std::unordered_set<combo_id>> exclude_tag(tag_id id) {
            return modify_tag_exclusion<true>(id);
        }


        bool is_combo_allowed(combo_id id) {
            if (id == 0) {
                return true;
            }

            Combo& combo = get_combo_data(id);
            return ((combo.exclusions == 0) && (combo.conflicts == 0));
        }

        bool is_combo_included(combo_id id) {
            if (id == 0) {
                return true;
            }

            Combo& combo = get_combo_data(id);
            return (combo.exclusions == 0);
        }

        const std::vector<combo_id>& get_related_combos(tag_id id) {
            Tag& tag = get_tag_data(id);
            return tag.related_combos;
        }
    }
}