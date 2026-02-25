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

        combo_id get_combo_id(const std::vector<std::string>& tagnames);
        combo_id get_combo_id(const char** tagnames, size_t tagcount);
        std::vector<tag_id> expand_combo(combo_id id);

        std::unordered_set<combo_id> reserve_combo(combo_id id);
        std::unordered_set<combo_id> free_combo(combo_id id);

        bool is_combo_allowed(combo_id id);
    }
}

#endif /* TAG_H */