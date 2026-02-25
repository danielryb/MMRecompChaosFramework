// Prototypes for ido math routines.
unsigned long long __ull_rem(unsigned long long left, unsigned long long right);
unsigned long long __ull_div(unsigned long long left, unsigned long long right);
long long __ll_rem(long long left, unsigned long long right);
long long __ll_div(long long left, long long right);
long long __ll_mod(long long left, long long right);
long long __d_to_ll(double d);
long long __f_to_ll(float f);
unsigned long long __d_to_ull(double d);
unsigned long long __f_to_ull(float f);
double __ll_to_d(long long s);
float __ll_to_f(long long s);
double __ull_to_d(unsigned long long u);
float __ull_to_f(unsigned long long u);

// Translations from libgcc math routines to ido ones.
unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    return __ull_rem(a, b);
}

unsigned long long int __udivdi3(unsigned long long int a, unsigned long long int b) {
    return __ull_div(a, b);
}

long long __moddi3(long long a, long long b) {
    return __ll_rem(a, b);
}

long long __divdi3 (long long a, long long b) {
    return __ll_div(a, b);
}

float __floatundisf(unsigned long long i) {
    return __ull_to_f(i);
}

double __floatundidf(unsigned long long i) {
    return __ull_to_d(i);
}

// clang doesn't allow inline asm for mips3 instructions in mips2 mode,
// so use naked functions with raw bytes containing the instructions.
__attribute__((naked)) double __floatdidf(long long i) {
    __asm__ __volatile__(
        "   .word 0x44850000\n" // mtc1    $a1, $fv0
        "   .word 0x44840800\n" // mtc1    $a0, $fv0f
        "   .word 0x03E00008\n" // jr      $ra
        "   .word 0x46A00021\n" // cvt.d.l $fv0, $fv0
    );
}

__attribute__((naked)) float __floatdisf(long long i) {
    __asm__ __volatile__(
        "   .word 0x44850000\n" // mtc1    $a1, $fv0
        "   .word 0x44840800\n" // mtc1    $a0, $fv0f
        "   .word 0x03E00008\n" // jr      $ra
        "   .word 0x46A00020\n" // cvt.s.l $fv0, $fv0
    );
}
