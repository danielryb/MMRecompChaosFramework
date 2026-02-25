using init_func = void(void);

extern init_func* __init_array_start[];
extern init_func* __init_array_end[];

asm(R"(
.section ".recomp_hook.recomp_entrypoint","axR",@progbits
.globl entrypoint_hook
.type entrypoint_hook,@function
.ent entrypoint_hook
entrypoint_hook:
    li $sp, 0x80800000
    j run_init_array
     nop
.end entrypoint_hook
.size entrypoint_hook, . - entrypoint_hook
.previous
)");

extern "C" void run_init_array() {
    for (init_func** cur_init_func = __init_array_start; cur_init_func != __init_array_end; cur_init_func++) {
        (*cur_init_func)();
    }
}

extern "C" int __cxa_atexit(void (*func) (void *), void *arg, void *d) {
    // There is no destruction process when the game stops (mod memory is entirely self-contained),
    // so there's no need to run global destructors.
    return 0;
}
