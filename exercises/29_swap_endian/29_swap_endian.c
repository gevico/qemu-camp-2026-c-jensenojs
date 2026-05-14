#include <stdint.h>
#include <stdio.h>

uint32_t swap_endian(uint32_t num) {
    /*
     * 大端/小端：指的是多字节数据在内存中的字节排列顺序。
     *
     * 小端（x86）：低地址存低位字节 → 0x78563412 在内存里是 12 34 56 78
     * 大端（网络）：低地址存高位字节 → 0x78563412 在内存里是 78 56 34 12
     *
     * 端序转换就是反转字节顺序，不改变字节内部的值。
     * 0x78563412 的四个字节：[0x78][0x56][0x34][0x12]
     * 反转后：               [0x12][0x34][0x56][0x78] = 0x12345678
     *
     * 实现方式：分别取出每个字节，左/右移到对应的新位置。
     */
    return ((num >> 24) & 0x000000FF) |   // 最高字节 → 最低位
           ((num >> 8)  & 0x0000FF00) |   // 次高字节 → 次低位
           ((num << 8)  & 0x00FF0000) |   // 次低字节 → 次高位
           ((num << 24) & 0xFF000000);    // 最低字节 → 最高位
}

int main(int argc, char* argv[]) {
    uint32_t num = 0x78563412;
    uint32_t swapped = swap_endian(num);
    printf("0x%08x -> 0x%08x\n", num, swapped);
    return 0;
}