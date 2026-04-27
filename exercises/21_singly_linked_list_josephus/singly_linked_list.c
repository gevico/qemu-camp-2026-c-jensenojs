#include "singly_linked_list.h"

#include <stdio.h>
#include <stdlib.h>

// 全局头指针
static link head = NULL;

// 创建新节点
link make_node(unsigned char item) {
    link node = (link)malloc(sizeof(struct node));
    if (node == NULL) return NULL;
    node->item = item;
    node->next = NULL;
    return node;
}

// 释放节点
void free_node(link p) { free(p); }

// 查找节点
link search(unsigned char key) {
    link curr = head;
    while (curr != NULL) {
        if (curr->item == key) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// 在链表头部插入节点
void insert(link p) {
    p->next = head;
    head = p;
}

// 删除指定节点
void delete(link p) {
    if (head == p) {
        head = head->next;
        return;
    }
    link curr = head;
    while (curr != NULL && curr->next != p) {
        curr = curr->next;
    }
    if (curr != NULL) {
        curr->next = p->next;
    }
}

// 遍历链表
void traverse(void (*visit)(link)) {
    link curr = head;
    while (curr != NULL) {
        visit(curr);
        curr = curr->next;
    }
}

// 销毁整个链表
void destroy(void) {
    link curr = head;
    while (curr != NULL) {
        link next = curr->next;
        free_node(curr);
        curr = next;
    }
    head = NULL;
}

// 在链表头部推入节点
void push(link p) {
    insert(p);
}

// 从链表头部弹出节点
link pop(void) {
    if (head == NULL) return NULL;
    link temp = head;
    head = head->next;
    return temp;
}

// 释放链表内存
void free_list(link list_head) {
    link curr = list_head;
    while (curr != NULL) {
        link next = curr->next;
        free_node(curr);
        curr = next;
    }
}
