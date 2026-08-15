#ifndef TAG_H
#define TAG_H

#include "util/debug.h"

#include <string>
#include <vector>
#include <unordered_set>

namespace Chaos {
    namespace Tag {
        using tag_id = int;
        using combo_id = int;

        void clear();

        tag_id get_tag_id(const std::string& tagname);
        bool add_tag(const std::string& tagname, size_t limit);
        combo_id get_combo_id(const std::vector<std::string>& tag_names);
        combo_id get_combo_id(const char* tag_names[], size_t tag_count);

        std::unordered_set<combo_id> reserve_combo(combo_id id);
        std::unordered_set<combo_id> free_combo(combo_id id);

        std::pair<bool, std::unordered_set<combo_id>> include_tag(tag_id id);
        std::pair<bool, std::unordered_set<combo_id>> exclude_tag(tag_id id);

        bool is_combo_allowed(combo_id id);
        bool is_combo_included(combo_id id);
        const std::vector<combo_id>& get_related_combos(tag_id id);
    }
}

#endif /* TAG_H */