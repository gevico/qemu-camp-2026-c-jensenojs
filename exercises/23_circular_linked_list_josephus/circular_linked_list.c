#include "circular_linked_list.h"

#include <stdio.h>
#include <stdlib.h>

Node* create_circular_list(int n) {
    if (n <= 0) return NULL;
    Node* head = NULL;
    Node* tail = NULL;
    for (int i = 1; i <= n; i++) {
        Node* new_node = (Node*)malloc(sizeof(Node));
        new_node->id = i;
        new_node->next = NULL;
        if (head == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }
    tail->next = head;
    return head;
}

void free_list(Node* head) {
    if (!head) return;
    // 找到尾节点并拆环
    Node* tail = head;
    while (tail->next != head) tail = tail->next;
    tail->next = NULL;
    // 现在可以线性遍历释放
    Node* current = head;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
}
