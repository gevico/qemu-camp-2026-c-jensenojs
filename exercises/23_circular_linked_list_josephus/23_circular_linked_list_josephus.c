#include <stdio.h>
#include <stdlib.h>

#include "circular_linked_list.h"

// 使用环形链表模拟约瑟夫环并打印出列顺序
static void josephus_problem(int n, int k, int m) {
    if (n <= 0 || k <= 0 || m <= 0) {
        printf("参数错误\n");
        return;
    }

    Node* head = create_circular_list(n);
    if (!head) {
        printf("\n");
        return;
    }

    // prev 指向 current 的前一个节点
    Node* current = head;
    Node* prev = head;
    while (prev->next != head) prev = prev->next;

    // 起始位置移动到第 k 个（1−based）
    for (int i = 1; i < k; ++i) {
        prev = current;
        current = current->next;
    }

    // 循环淘汰：每数到 m 就移除当前节点，直到只剩一个
    Node* tmp = NULL;
    while (current->next != current) {
        // 前移 m-1 步，使 current 指向要淘汰的节点
        for (int i = 1; i < m; ++i) {
            prev = current;
            current = current->next;
        }
        // 淘汰 current
        printf("%d ", current->id);
        tmp = current;
        current = current->next;
        prev->next = current;
        free(tmp);
    }
    // 输出最后一个幸存者
    printf("%d ", current->id);
    free(current);

    printf("\n");
}

int main(void) {
    josephus_problem(5, 1, 2);  // 输出结果：2 4 1 5 3
    josephus_problem(7, 3, 1);  // 输出结果：3 4 5 6 7 1 2
    josephus_problem(9, 1, 8);  // 输出结果：8 7 9 2 5 4 1 6 3
    return 0;
}
