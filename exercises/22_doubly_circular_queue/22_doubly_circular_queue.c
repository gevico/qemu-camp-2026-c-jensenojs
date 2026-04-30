// main.c
#include <stdio.h>
#include <stdlib.h>

#include "doubly_circular_queue.h"

// 使用通用双向链表实现队列：
// - 入队：头插（insert(make_node(v)))
// - 出队：删除最后一个结点（tail 前的结点）

static link g_last = NULL;
static void capture_last(link p) { g_last = p; }
static link get_last(void) {
    g_last = NULL;
    traverse(capture_last);
    return g_last;  // 若为空表示队列为空
}

static void enqueue_int(int v) {
    link node = make_node(v );
    if(node) insert(node);
}

static int dequeue_int(int *out) {
    link last = get_last();
    if (!last) return 0;
    /* delete the last element itself (the node before tail), not its prev */
    link node = last;
    if (out) *out = node->data;
    delete(node);
    return 1;
}

static void print_dequeue_n(int n) {
    int i;
    for(i = 0; i < n; i++) {
        int v;
        if(!dequeue_int(&v)) break;
        printf("%d ", v);
    }
}

int main(void) {
    // 基本 FIFO 顺序：1 2 3
    enqueue_int(1);
    enqueue_int(2);
    enqueue_int(3);
    print_dequeue_n(3);
    printf("\n");
    destroy();

    // 单元素队列：42
    enqueue_int(42);
    printf("single: ");
    print_dequeue_n(1);
    printf("\n");
    destroy();

    // 空队列出队：OK（0 代表出队失败）
    {
        int ok = dequeue_int(NULL);
        printf("empty: %s\n", ok ? "NG" : "OK");
    }
    destroy();

    return 0;
}
