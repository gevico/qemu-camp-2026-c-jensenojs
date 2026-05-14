#include <stdio.h>

unsigned int gcd_asm(unsigned int a, unsigned int b) {
    unsigned int result;

    /*
     * GCC 扩展内联汇编语法（非标准 C，GCC 专有扩展）：
     *
     *   __asm__ volatile ( "指令模板" : 输出操作数 : 输入操作数 : 破坏列表 );
     *
     * - volatile：禁止编译器认为"输出没被用"而删除 asm 块
     * - %0, %1, %2：分别引用第 0/1/2 个操作数（按 输出→输入 顺序编号）
     * - %%eax：两个百分号表示字面寄存器名（单百分号被保留给操作数引用）
     * - "r"(x)：把 x 放到任意通用寄存器
     * - "=r"(x)：输出到任意通用寄存器（等号=表示写-only）
     * - "a"：强制使用 eax 寄存器
     * - 破坏列表：告诉编译器我们修改了哪些寄存器，让它保存恢复
     *
     * 这里的汇编是 AT&T 语法（GCC 在 x86 上的默认风格）。
     * 和 Intel 语法的关键差异：
     *              AT&T（本文件） Intel
     *   操作数顺序  mov src, dst   mov dst, src
     *   寄存器前缀  %%eax          eax
     *   立即数前缀  $42            42
     * AT&T 的 "mov %1, %%eax" 等价于 Intel 的 "mov eax, %1"。
     */
    __asm__ volatile (
        "mov %1, %%eax\n\t"     // %1 = a → eax
        "mov %2, %%ebx\n\t"     // %2 = b → ebx
        "jmp .L_check\n\t"

        /*
         * 最大公约数（GCD）：能同时整除两个数的最大整数。
         * 例：12 和 8 的公约数有 1、2、4，最大是 4，所以 gcd(12,8)=4。
         *
         * 实际用途其实很朴素：
         *   1) 约分：12/8 分子分母同除以 4 → 3/2
         *   2) 分组：要把 12 个红球和 8 个蓝球均匀分到若干组，
         *      每组红蓝比例相同，最多能分 gcd(12,8)=4 组，
         *      每组 3 红 2 蓝
         *
         * 欧几里得算法（公元前 300 年）就是快速算 GCD 的方法，
         * 不需要列出所有公约数。原理一句话：
         *   两个数的最大公约数 = 较小的数和"大数除小数的余数"的最大公约数。
         *
         * 例：gcd(12, 8)
         *   12 % 8 = 4 → gcd(8, 4)   （12 和 8 的 gcd 等于 8 和 4 的 gcd）
         *   8  % 4 = 0 → gcd(4, 0) = 4（余数为 0 时答案就是当前除数）
         *
         * 用 C 写就是：
         *   while (b != 0) {
         *       unsigned r = a % b;
         *       a = b;
         *       b = r;
         *   }
         *   return a;
         *
         * 下面用汇编实现这个循环：
         *   a → eax（被除数兼累积结果）
         *   b → ebx（除数）
         *   a % b → edx（余数）
         *   每轮迭代：edx:eax 除以 ebx，商在 eax，余数在 edx
         *
         * DIV 指令约定（32 位模式）：
         *   被除数：edx:eax（64 位，高 32 位在 edx）
         *   除数：  任意寄存器或内存（这里是 ebx）
         *   商 → eax，余数 → edx
         *
         * 所以每次迭代前必须清零 edx（xor %%edx,%%edx），
         * 否则 edx 里的残留值会影响除法结果。
         */
        ".L_loop:\n\t"
        "   xor %%edx, %%edx\n\t"
        "   div %%ebx\n\t"          // eax = a/b, edx = a%b
        "   mov %%ebx, %%eax\n\t"   // a = b（旧除数成为新被除数）
        "   mov %%edx, %%ebx\n\t"   // b = remainder

        ".L_check:\n\t"
        "   test %%ebx, %%ebx\n\t"  // b == 0？
        "   jne .L_loop\n\t"        // 非零继续

        "mov %%eax, %0"             // eax → result
        : "=r" (result)             // %0：输出到任意寄存器
        : "r" (a), "r" (b)          // %1=a, %2=b，编译器选寄存器
        : "eax", "ebx", "edx"       // 这三个寄存器的值被 asm 覆盖了
    );

    return result;
}

int main(int argc, char* argv[]) {
    printf("%d\n", gcd_asm(12, 8));
    printf("%d\n", gcd_asm(7, 5));
    return 0;
}