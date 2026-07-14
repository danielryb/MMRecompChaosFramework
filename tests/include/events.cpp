#include "events.h"

std::function<void()> init_fun_lambda;

void Chaos::chaos_on_init() {
    init_fun_lambda();
}

void Chaos::set_on_init(std::function<void()>&& fun) {
    init_fun_lambda = std::move(fun);
}