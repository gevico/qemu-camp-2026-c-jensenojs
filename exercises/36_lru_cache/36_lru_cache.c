#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 16 LRU 缓存淘汰算法（哈希表 + 双向链表）
 *  - put(k,v)、get(k) 均为 O(1)
 *  - 容量满时淘汰最久未使用（LRU）的元素
 */

typedef struct LRUNode {
    int key;
    int value;
    struct LRUNode* prev;
    struct LRUNode* next;
} LRUNode;

typedef struct HashEntry {
    int key;
    LRUNode* node;
    struct HashEntry* next;
} HashEntry;

typedef struct {
    int capacity;
    int size;
    LRUNode* head; /* 最近使用（MRU） */
    LRUNode* tail; /* 最久未使用（LRU） */
    /* 简单链式哈希表 */
    size_t bucket_count;
    HashEntry** buckets;
} LRUCache;

static unsigned hash_int(int key) {
    /* 简单的取模哈希 */
    return (unsigned)(key < 0 ? -key : key);
}

static HashEntry* hash_find(LRUCache* c, int key, HashEntry*** pprev_next) {
    unsigned idx = hash_int(key) % c->bucket_count;
    HashEntry** pp = &c->buckets[idx];
    while (*pp) {
        if ((*pp)->key == key) {
            if (pprev_next) *pprev_next = pp;
            return *pp;
        }
        pp = &(*pp)->next;
    }
    if (pprev_next) *pprev_next = pp;
    return NULL;
}

static void list_add_to_head(LRUCache* c, LRUNode* node) {
    node->prev = NULL;
    node->next = c->head;
    if (c->head) c->head->prev = node;
    c->head = node;
    if (!c->tail) c->tail = node;
}

static void list_remove(LRUCache* c, LRUNode* node) {
    if (node->prev) node->prev->next = node->next;
    else c->head = node->next;
    if (node->next) node->next->prev = node->prev;
    else c->tail = node->prev;
}

static void list_move_to_head(LRUCache* c, LRUNode* node) {
    if (node == c->head) return;
    list_remove(c, node);
    list_add_to_head(c, node);
}

static LRUNode* list_pop_tail(LRUCache* c) {
    if (!c->tail) return NULL;
    LRUNode* t = c->tail;
    list_remove(c, t);
    return t;
}

/* LRU 接口实现 */
static LRUCache* lru_create(int capacity) {
    LRUCache* c = calloc(1, sizeof(LRUCache));
    if (!c) return NULL;
    c->capacity = capacity;
    c->bucket_count = (size_t)capacity * 2 + 1;
    c->buckets = calloc(c->bucket_count, sizeof(HashEntry*));
    if (!c->buckets) {
        free(c);
        return NULL;
    }
    return c;
}

static void lru_free(LRUCache* c) {
    if (!c) return;
    /* 释放所有哈希表项 */
    for (size_t i = 0; i < c->bucket_count; i++) {
        HashEntry* e = c->buckets[i];
        while (e) {
            HashEntry* tmp = e;
            e = e->next;
            free(tmp);
        }
    }
    /* 释放所有 LRU 节点 */
    LRUNode* p = c->head;
    while (p) {
        LRUNode* tmp = p;
        p = p->next;
        free(tmp);
    }
    free(c->buckets);
    free(c);
}

static int lru_get(LRUCache* c, int key, int* out_value) {
    HashEntry* e = hash_find(c, key, NULL);
    if (!e) return 0;
    list_move_to_head(c, e->node);
    if (out_value) *out_value = e->node->value;
    return 1;
}

/* 如果缓存已满，淘汰 LRU 元素（队尾）。应紧跟在插入后调用 */
static void lru_evict_if_full(LRUCache* c) {
    if (c->size <= c->capacity) return;
    LRUNode* victim = list_pop_tail(c);
    if (!victim) return;
    /* 从哈希表删除对应项 */
    HashEntry** pp;
    hash_find(c, victim->key, &pp);
    HashEntry* to_del = *pp;
    *pp = to_del->next;
    free(to_del);
    free(victim);
    c->size--;
}

static void lru_put(LRUCache* c, int key, int value) {
    HashEntry* e = hash_find(c, key, NULL);
    if (e) {
        /* key 已存在：更新值，移至 MRU 位置 */
        e->node->value = value;
        list_move_to_head(c, e->node);
        return;
    }
    /* 创建新节点，插入链表头部（MRU） */
    LRUNode* n = malloc(sizeof(LRUNode));
    n->key = key;
    n->value = value;
    n->prev = n->next = NULL;
    list_add_to_head(c, n);
    /* 创建哈希表项，链入对应桶 */
    HashEntry* he = malloc(sizeof(HashEntry));
    he->key = key;
    he->node = n;
    unsigned idx = hash_int(key) % c->bucket_count;
    he->next = c->buckets[idx];
    c->buckets[idx] = he;
    c->size++;
    /* 超出容量则淘汰 */
    lru_evict_if_full(c);
}

/* 打印当前缓存内容（从头到尾） */
static void lru_print(LRUCache* c) {
    LRUNode* p = c->head;
    int first = 1;
    while (p) {
        if (!first) printf(", ");
        first = 0;
        printf("%d:%d", p->key, p->value);
        p = p->next;
    }
    printf("\n");
}

int main(void) {
    /* 容量 3：put(1,1), put(2,2), put(3,3), put(4,4), get(2), put(5,5) */
    LRUCache* c = lru_create(3);
    if (!c) {
        fprintf(stderr, "创建 LRU 失败\n");
        return 1;
    }

    lru_put(c, 1, 1); /* 缓存：1 */
    lru_put(c, 2, 2); /* 缓存：2,1 */
    lru_put(c, 3, 3); /* 缓存：3,2,1 (满) */
    lru_put(c, 4, 4); /* 淘汰 LRU(1)，缓存：4,3,2 */

    int val;
    if (lru_get(c, 2, &val)) {
        /* 访问 2：缓存：2,4,3 */
        (void)val; /* 演示无需使用 */
    }

    lru_put(c, 5, 5); /* 淘汰 LRU(3)，缓存：5,2,4 */

    /* 期望最终键集合：{2,4,5}，顺序无关。此处按最近->最久打印：5:5, 2:2, 4:4 */
    lru_print(c);

    lru_free(c);
    return 0;
}
