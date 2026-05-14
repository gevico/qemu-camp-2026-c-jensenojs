#include <stddef.h>  // offsetof
#include <stdio.h>

/*
 * 12 container_of 宏实现
 * 要求：
 *  - 使用 GCC 扩展（typeof、语句表达式）实现 container_of(ptr, type, member)
 *  - 通过结构体成员的指针获取结构体变量的指针
 *  - 示例：struct Test { int a; char b; }，当 ptr=&t.b 时应返回 &t
 */

struct Test {
    int a;
    char b;
};

/*
 * container_of 宏
 * 原理：结构体首地址 + 成员偏移量 = 成员地址
 *      → 结构体首地址 = 成员地址 - 成员偏移量
 *
 *         struct Test {
 *             int  a;   // offset=0
 *             char b;   // offset=4  （int 对齐后）
 *         };
 *         &t     t.b     &t.b - offsetof(struct Test, b) = &t
 *          ↓      ↓
 *         [ a ][ b ]     b 的地址减去 4 就回到了 t 的开头
 *
 * offsetof(type, member) 是 stddef.h 提供的宏，返回成员在结构体中的字节偏移。
 * 它的典型实现是 ((size_t)&((type*)0)->member)——假装从地址 0 开始布局，
 * 取成员的地址，这个地址值就是偏移量（因为基址是 0）。
 *
 * typeof 是 GCC 扩展，用于声明一个与 ptr 类型匹配的临时指针 __mptr。
 * 如果 ptr 的类型不是 type→member 的类型，编译器会报 warning。
 * 这是一个编译期类型检查——没有运行时开销。
 *
 * 整个宏被包在 ({...}) 中（GCC 语句表达式），
 * 最后一行 (type*)... 的值作为整个表达式的返回值。
 * 这也是 GCC 扩展，不在 C 标准里。
 */
#define container_of(ptr, type, member) ({                     \
    const typeof(((type*)0)->member) *__mptr = (ptr);          \
    (type*)((char*)__mptr - offsetof(type, member));           \
})

int main(void) {
    struct Test t = {.a = 42, .b = 'Z'};

    /* 取成员 b 的指针 */
    char *ptr_to_b = &t.b;

    /* 使用 container_of 通过成员指针反推结构体指针 */
    struct Test *owner = container_of(ptr_to_b, struct Test, b);

    /* 输出两个地址，应当一致（测试程序将解析并验证） */
    printf("container_of(ptr_to_b) = %p\n", (void *)owner);
    printf("&t                  = %p\n", (void *)&t);

    /* 简单校验：相等则返回 0 */
    if (owner != &t) {
        fprintf(stderr, "校验失败：地址不一致\n");
        return 1;
    }
    return 0;
}
