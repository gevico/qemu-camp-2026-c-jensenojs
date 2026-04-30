#include "doubly_circular_queue.h"

#include <stdlib.h>

// 头尾哨兵
static struct node tailsentinel;
static struct node headsentinel = {0, NULL, &tailsentinel};
static struct node tailsentinel = {0, &headsentinel, NULL};

static link head = &headsentinel;
static link tail = &tailsentinel;

link make_node(int data) {
    link node = (link)malloc(sizeof(struct node));
    if (node == NULL) return NULL;
    node->data = data;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

void free_node(link p) {
    link prev = p->prev, next = p->next;
    prev->next = next, next->prev = prev;
    free(p);
}

link search(int key) {
    link current = head->next;
    while (current != tail) {
        if (current->data == key) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 需要使用头插法
void insert(link p) {
    p->next = head->next;
    p->prev = head;
    head->next->prev = p;
    head->next = p;
}

void delete(link p) {
    link prev = p->prev, next = p->next;
    prev->next = next, next->prev = prev;
    free(p);
}

void traverse(void (*visit)(link)) {
    link curr = head->next;
    while (curr != tail) {
        /*
         * Save next before calling visit() because visit() may free
         * the current node (e.g. free_node). Reading curr->next after
         * visit() can lead to use-after-free. Precompute next here
         * to make traversal safe when visitors mutate/free nodes.
         */
        link next = curr->next;
        visit(curr);
        curr = next;
    }
}

void destroy(void) {
    traverse(free_node);
}
