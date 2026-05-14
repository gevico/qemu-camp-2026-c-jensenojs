#include <limits.h>
#include <stdio.h>

#define CHECK_OVERFLOW(carry) carry ? "Overflow" : "Not Overflow"

int check_add_overflow_asm(unsigned int a, unsigned int b) {
    unsigned char carry;
    /*
     * 无符号加法溢出：结果超过 32 位时进位标志 CF（Carry Flag）置 1。
     * ADD 指令执行后 CPU 自动设置 CF，用 SETC 读出。
     *
     * ┌─────────────────────────────────────────────────────────────────┐
     * │ 操作数编号规则（GCC 最容易迷惑新手的地方）                      │
     * │                                                                 │
     * │ 所有 %0 %1 %2 是全局连续编号的——不分"输出段""输入段"。          │
     * │ 先编所有输出（包括 +r 读写），再编所有输入。                    │
     * │                                                                 │
     * │ 本例的编号对应关系：                                            │
     * │   : "=r"(carry)   → %0  （输出，setc 写入）                     │
     * │     "+r"(a)       → %1  （输出+输入，add 既读又写）             │
     * │   : "r"(b)        → %2  （输入，add 只读）                      │
     * │                                                                 │
     * │ 所以模板里：                                                    │
     * │   "add %2, %1"  等价于  a = a + b（AT&T 源在前，目标在后）      │
     * │   "setc %0"     等价于  carry = CF                              │
     * │                                                                 │
     * │ 编译器在编译时查空闲寄存器池，给 %0/%1/%2 各分配一个寄存器，    │
     * │ 然后替换成真实寄存器名。这段 asm 没有硬编码 %%eax               │
     * │ 这样的寄存器名，全部用占位符，所以破坏列表（第四个冒号）为空。  │
     * └─────────────────────────────────────────────────────────────────┘
     */
    __asm__ volatile(
        "add %2, %1\n\t"  // %1 += %2 —— 即 a += b
        "setc %0\n\t"     // %0 = CPU进位标志 CF —— 即 carry = 溢出?
        : "=r"(carry), "+r"(a)
        : "r"(b)
        :);
    return carry;
}

int check_sub_overflow_asm(unsigned int a, unsigned int b) {
    unsigned char carry;
    /*
     * 无符号减法借位：a < b 时不够减，CPU 置 CF 为 1。
     * 和加法用同一个 CF，但语义不同——加法叫"进位"，减法叫"借位"，
     * CPU 内部统一用 CF 表示。
     */
    __asm__ volatile(
        "sub %2, %1\n\t"  // a -= b（CF 置 1 当 a < b）
        "setc %0\n\t"     // carry = CF（借位时为 1）
        : "=r"(carry), "+r"(a)
        : "r"(b)
        :);
    return carry;
}

int check_mul_overflow_asm(unsigned int a, unsigned int b) {
    unsigned int high_bits;
    unsigned char overflow;
    /*
     * 无符号乘法溢出：MUL 指令的结果是 edx:eax 拼成的 64 位值。
     * 低 32 位在 eax，高 32 位在 edx。
     * 如果 edx != 0，说明乘积超过 32 位 → 溢出。
     */
    __asm__ volatile(
        "mov %2, %%eax\n\t"  // a → eax
        "mul %3\n\t"         // edx:eax = eax * b（64 位结果）
        "mov %%edx, %0\n\t"  // high_bits = edx（高 32 位）
        "xor %1, %1\n\t"     // overflow = 0（实际由 high_bits 判断）
        : "=r"(high_bits), "=r"(overflow)
        : "r"(a), "r"(b)
        : "eax", "edx");
    return overflow || (high_bits != 0);
}

int check_div_overflow_asm(unsigned int a, unsigned int b) {
    unsigned char is_div_zero;
    /*
     * 除零检测：DIV b 时如果 b = 0 会触发 CPU 异常（#DE）。
     * 不能真的执行 DIV，必须在除法前判零。
     * TEST 置零标志 ZF，SETZ 读出。
     */
    __asm__ volatile(
        "test %1, %1\n\t"  // b == 0 ? 置 ZF
        "setz %0\n\t"      // is_div_zero = ZF（b == 0 时为 1）
        : "=r"(is_div_zero)
        : "r"(b)
        :);
    return is_div_zero;
}

int main() {
    printf("(UINT_MAX + 1)Add: %s\n", CHECK_OVERFLOW(check_add_overflow_asm(UINT_MAX, 1)));  // 1
    printf("(1, 0)Add: %s\n", CHECK_OVERFLOW(check_add_overflow_asm(1, 0)));
    printf("(0, 1)Sub: %s\n", CHECK_OVERFLOW(check_sub_overflow_asm(0, 1)));  // 1
    printf("(2, 1)Sub: %s\n", CHECK_OVERFLOW(check_sub_overflow_asm(2, 1)));
    printf("(UINT_MAX, 2)Mul: %s\n", CHECK_OVERFLOW(check_mul_overflow_asm(UINT_MAX, 2)));  // 1
    printf("(1, 2)Mul: %s\n", CHECK_OVERFLOW(check_mul_overflow_asm(1, 2)));
    printf("(10, 0)Div: %s\n", CHECK_OVERFLOW(check_div_overflow_asm(10, 0)));  // 1
    printf("(2, 1)Div: %s\n", CHECK_OVERFLOW(check_div_overflow_asm(2, 1)));
    return 0;
}
