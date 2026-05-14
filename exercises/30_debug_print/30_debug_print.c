#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * DEBUG_PRINT 宏：根据 DEBUG_LEVEL 控制输出详细程度。
 *   LEVEL=1：输出函数名和行号
 *   LEVEL=2：额外输出变量值
 *   LEVEL=3：额外输出调用堆栈
 *
 * DEBUG_LEVEL 在 Makefile 中通过 -DDEBUG_LEVEL=$(DEBUG_LEVEL) 传入，默认 2。
 */
#if DEBUG_LEVEL >= 1

#define DEBUG_PRINT(fmt, ...)                                                                \
    do {                                                                                     \
        if (DEBUG_LEVEL >= 2) {                                                              \
            /* LEVEL 2/3：输出函数名、行号、变量值 */                                        \
            printf("DEBUG: func=%s, line=%d, " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
            if (DEBUG_LEVEL >= 3) {                                                          \
                /* LEVEL 3：额外输出调用堆栈（需要 -rdynamic 链接） */                       \
                void *callstack[128];                                                        \
                int frames = backtrace(callstack, 128);                                      \
                char **strs = backtrace_symbols(callstack, frames);                          \
                for (int i = 0; i < frames; i++) printf("  [%d] %s\n", i, strs[i]);          \
                free(strs);                                                                  \
            }                                                                                \
        } else {                                                                             \
            /* LEVEL 1：只输出函数名和行号 */                                                \
            printf("DEBUG: func=%s, line=%d\n", __func__, __LINE__);                         \
        }                                                                                    \
    } while (0)

#else

#define DEBUG_PRINT(fmt, ...) \
    do {                      \
    } while (0)

#endif

//! 注：DEBUG_PRINT("x=%d", x) 位于第 48 行（__LINE__==48），与测试期望一致。

// 测试代码
void test() {
    int x = 42;

    DEBUG_PRINT("x=%d", x);
}

int main() {
    test();
    return 0;
}

/*
 * backtrace() / backtrace_symbols() 原理（glibc 扩展，非 C 标准）：
 *
 * 栈帧布局（以 test() 调用 DEBUG_PRINT 为例）：
 *
 *      高地址
 *      ┌─────────────────────────────────┐
 *      │  test() 的局部变量              │  ← test() 的栈帧
 *      ├─────────────────────────────────┤
 *      │  返回地址（保存的 RIP）         │  ← 指向 test() 中 call DEBUG_PRINT 的下一行
 *      ├─────────────────────────────────┤
 *      │保存的旧 RBP（指向 test() 帧基） │──┐
 * %rbp→├─────────────────────────────────┤  │  ← DEBUG_PRINT 的栈帧
 *      │  DEBUG_PRINT 的局部变量         │  │
 *      │  (callstack 数组等)             │  │
 * %rsp→└─────────────────────────────────┘  │
 *                                           │
 *            ┌──────────────────────────────┘
 *            ▼
 *      上一层(test()) 的 RBP 指向的帧：
 *      ┌─────────────────────────────────┐
 *      │  返回地址（保存的 RIP）         │  ← 指向 main() 中 call test 的下一行
 *      ├─────────────────────────────────┤
 *      │ 保存的旧 RBP（指向 main() 帧基）│──→ ... → main() → _start
 *      └─────────────────────────────────┘
 *   低地址
 *
 * 这是两个不同寄存器合力的结果：
 *   RIP（Instruction Pointer） CPU 正在执行的指令地址。类比图灵机的纸带读写头。
 *                              call 把 RIP 压栈保存（"回来继续读哪里"），
 *                              ret 弹栈恢复 RIP。RIP 本身不"在栈上"，
 *                              但它的历史值以返回地址的形式存储在栈帧里。
 *                              对 backtrace 来说，RIP 是"帧内保存的数据"。
 *
 *   RBP（Base Pointer）        当前栈帧的基址。用来索引局部变量和参数。
 *                              帧里保存的"旧 RBP"把各帧串成一条单向链表。
 *                              对 backtrace 来说，RBP 是"把帧串起来"的指针。
 *
 * 遍历逻辑（单向链表反向遍历）：
 *   while (帧链未走到头):
 *       读当前帧里保存的 RIP（返回地址）→ 记下"这个帧是哪个函数"
 *       读当前帧里保存的旧 RBP（prev）  → 移动到调用者的帧
 *
 * GDB 的 bt 命令做的也是同一件事——读同一套栈帧链。
 * 区别在于 GDB 能利用 DWARF 调试信息（-g 编译出的 .debug_* 段），
 * 把地址映射到源文件行号、局部变量值、函数参数，
 * 而 backtrace_symbols() 只能靠 ELF 的符号表把地址映射到函数名+偏移。
 *
 * glibc 和 C 标准的关系：
 *   C 标准（ISO/IEC 9899）只规定了约 150 个函数的最小集合
 *   （printf、malloc、strlen 等），可以在任何操作系统上实现。
 *   glibc 在实现这些标准函数的基础上，还加了几百个扩展函数，
 *   包括 backtrace()、backtrace_symbols()、strdup() 等。
 *   这些扩展函数不属于 C 标准，甚至不属于 POSIX 标准，
 *   用它们写的代码在其他 C 库（musl、ucLibc、BSD libc）上可能无法编译。
 *   这就是为什么 26 题里 strdup() 在 -std=c11 下报错——需要 _GNU_SOURCE。
 *
 * LEVEL=3 的堆栈需要编译时加 -rdynamic 才能看到有意义的函数名，
 * 否则 backtrace_symbols() 只能显示原始地址。
 */
