#include "simple_tree.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

Queue *create_queue() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(Queue *q, TreeNode *tree_node) {
    QueueNode *rear = q->rear;
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    new_node->tree_node = tree_node;
    new_node->next = NULL;
    if (rear == NULL) {
        q->front = new_node;
    } else {
        rear->next = new_node;
    }
    q->rear = new_node;
}

TreeNode *dequeue(Queue *q) {
    if (is_empty(q)) {
        return NULL;
    }
    QueueNode *old_front = q->front;
    TreeNode *to_pop = old_front->tree_node;
    q->front = old_front->next;
    if (q->front == NULL) {
        q->rear = NULL;  // 队列变空时，rear 也要跟着置空
    }
    free(old_front);
    return to_pop;
}

bool is_empty(Queue *q) { return q->front == NULL; }

void free_queue(Queue *q) {
    while (!is_empty(q)) {
        dequeue(q);
    }
    free(q);
}

TreeNode *build_tree_by_level(int *level_order, int size) {
    if (size <= 0) return NULL;

    Queue *q = create_queue();
    TreeNode *root = (TreeNode *)malloc(sizeof(TreeNode));
    root->val = level_order[0];
    root->left = root->right = NULL;
    enqueue(q, root);

    int i = 1;
    while (!is_empty(q) && i < size) {
        TreeNode *parent = dequeue(q);

        // 左孩子
        if (level_order[i] != INT_MIN) {
            TreeNode *left = (TreeNode *)malloc(sizeof(TreeNode));
            left->val = level_order[i];
            left->left = left->right = NULL;
            parent->left = left;
            enqueue(q, left);
        }
        i++;

        // 右孩子
        if (i < size) {
            if (level_order[i] != INT_MIN) {
                TreeNode *right = (TreeNode *)malloc(sizeof(TreeNode));
                right->val = level_order[i];
                right->left = right->right = NULL;
                parent->right = right;
                enqueue(q, right);
            }
            i++;
        }
    }

    return root;
}

void preorder_traversal(TreeNode *root) {
    if (root == NULL) return;
    printf("%d ", root->val);
    preorder_traversal(root->left);
    preorder_traversal(root->right);
}

void preorder_traversal_iterative(TreeNode *root) {
    if (root == NULL) return;
    // 显式栈模拟递归
    TreeNode *stack[256];
    int top = -1;
    stack[++top] = root;
    while (top >= 0) {
        TreeNode *node = stack[top--];
        printf("%d ", node->val);
        if (node->right) stack[++top] = node->right;
        if (node->left) stack[++top] = node->left;
    }
}

void free_tree(TreeNode *root) {
    if (root == NULL) {
        return;
    }
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}
