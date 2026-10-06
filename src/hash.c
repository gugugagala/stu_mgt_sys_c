/*
 * hash.c — 学号哈希表实现
 *
 * 链地址法 + 动态扩容：
 *   - 初始容量 1024，负载因子 > 0.75 时扩容为 2 倍
 *   - 哈希函数: id % capacity
 *   - 冲突链通过 Student.hash_next 串联，遍历 O(链长)
 */
#include "hash.h"

#define INITIAL_CAPACITY 1024
#define LOAD_FACTOR      0.75f

static Student **buckets = NULL;
static int       capacity = 0;
static int       count    = 0;

static void resize(void) {
    int old_cap = capacity;
    Student **old = buckets;

    capacity = (old_cap == 0) ? INITIAL_CAPACITY : old_cap * 2;
    buckets = calloc((size_t)capacity, sizeof(Student*));
    if (!buckets) {
        /* 扩容失败则保留旧表继续运行 */
        buckets = old;
        capacity = old_cap;
        return;
    }
    count = 0;

    for (int i = 0; i < old_cap; i++) {
        Student *node = old[i];
        while (node) {
            Student *next = node->hash_next;
            int idx = node->id % capacity;
            node->hash_next = buckets[idx];
            buckets[idx] = node;
            count++;
            node = next;
        }
    }
    free(old);
}

void hash_init(void) {
    capacity = INITIAL_CAPACITY;
    buckets = calloc((size_t)capacity, sizeof(Student*));
    count = 0;
}

void hash_destroy(void) {
    free(buckets);
    buckets = NULL;
    capacity = 0;
    count = 0;
}

Student* hash_find(int id) {
    if (!buckets) {
        return NULL;
    }
    int idx = id % capacity;
    for (Student *p = buckets[idx]; p; p = p->hash_next) {
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}

void hash_insert(Student *node) {
    if (!buckets) hash_init();

    /* 扩容检查 */
    if ((float)(count + 1) / capacity > LOAD_FACTOR)
        resize();

    int idx = node->id % capacity;
    node->hash_next = buckets[idx];
    buckets[idx] = node;
    count++;
}

void hash_remove(int id) {
    if (!buckets) {
        return;
    }
    int idx = id % capacity;
    Student **pp = &buckets[idx];
    while (*pp) {
        if ((*pp)->id == id) {
            *pp = (*pp)->hash_next;
            count--;
            return;
        }
        pp = &(*pp)->hash_next;
    }
}
