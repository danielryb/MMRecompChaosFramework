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

        tag_id next_tag_id = FIRST_TAG_ID;
        combo_id next_combo_id = FIRST_COMBO_ID;

        std::unordered_map<std::string, tag_id> tags;
        std::map<std::vector<tag_id>, combo_id> combos;
        std::vector<std::vector<tag_id>> expanded_combos; // combo -> expanded combo
                                                          // in the form of vector<tag>.
        std::vector<std::vector<combo_id>> related_combos;  // tag -> vector of combos
                                                            // containing this tag.
        std::vector<bool> tag_reservations;

        inline size_t get_tag_pos(tag_id id) {
            return id - FIRST_TAG_ID;
        }

        inline size_t get_combo_pos(combo_id id) {
            return id - FIRST_COMBO_ID;
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
                related_combos.emplace_back();
                tag_reservations.push_back(false);
            }
            return id;
        }

        combo_id get_combo_id(std::vector<tag_id>&& combo) {
            combo_id prev_next_id = next_combo_id;
            combo_id id = get_id(combo, next_combo_id, combos);

            if (prev_next_id != next_combo_id) {
                for (auto tag : combo) {
                    related_combos[get_tag_pos(tag)].push_back(id);
                }
                expanded_combos.emplace_back(combo);
            }
            return id;
        }

        combo_id get_combo_id(const std::vector<std::string>& tagnames) {
            std::vector<tag_id> combo;

            for (const std::string& tagname : tagnames) {
                combo.push_back(get_tag_id(tagname));
            }
            // TODO Replace.
            // std::sort(combo.begin(), combo.end());

            return get_combo_id(std::move(combo));
        }

        combo_id get_combo_id(const char** tagnames, size_t tagcount) {
            if (tagcount == 0) {
                return 0;
            }

            std::vector<std::string> vec;
            for (size_t i = 0; i < tagcount; i++) {
                const char* tagname = tagnames[tagcount];
                vec.emplace_back(tagname);
            }
            return get_combo_id(vec);
        }

        std::vector<tag_id> expand_combo(combo_id id) {
            // TODO add case for combo id = 0.
            return expanded_combos[get_combo_pos(id)];
        }


        std::unordered_set<combo_id> modify_reservations(combo_id id, bool val) {
            std::unordered_set<combo_id> affected_combos;

            if (id == 0) {
                return affected_combos;
            }

            auto& expanded_combo = expanded_combos[get_combo_pos(id)];
            for (auto tag : expanded_combo) {
                tag_reservations[get_tag_pos(tag)] = val;

                // this could be replaced with a function which maps
                // combo -> vector of related combos (sharing at least one tag).
                // TODO consider.
                auto& related = related_combos[get_tag_pos(tag)];
                affected_combos.insert(related.begin(), related.end());
            }

            return affected_combos;
        }

        std::unordered_set<combo_id> reserve_combo(combo_id id) {
            return modify_reservations(id, true);
        }

        std::unordered_set<combo_id> free_combo(combo_id id) {
            return modify_reservations(id, false);
        }

        bool is_combo_allowed(combo_id id) {
            if (id == 0) {
                return true;
            }

            auto& expanded_combo = expanded_combos[get_combo_pos(id)];
            for (auto tag : expanded_combo) {
                if (tag_reservations[get_tag_pos(tag)]) {
                    return false;
                }
            }
            return true;
        }
    }
}