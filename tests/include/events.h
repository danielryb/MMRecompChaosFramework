#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <functional>

namespace Chaos {
    void chaos_on_init();
    void set_on_init(std::function<void()>&& fun);
}

#endif /* __EVENTS_H__ */