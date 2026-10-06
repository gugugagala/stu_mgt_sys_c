/*
 * sort.c — 排序模块
 *
 * 实现六种排序算法及排序入口。所有算法操作单向链表，
 * 通过公共比较函数 cmp_fields 统一比较规则。
 * 排序入口提供交互式菜单，执行后显示耗时。
 */

#include "sort.h"
#include <time.h>

/* ============================================================
 * 排序辅助 — 比较两个结点的指定字段
 *
 * 根据 field（1=学号 2=姓名 3=年龄 4=成绩）和 order（1=升序 2=降序）
 * 比较 a、b 两个结点。返回值 <0 表示 a 应排在 b 前面，>0 表示 b 应排在前面。
 * 所有排序算法共用此函数，避免重复编写比较逻辑。
 * ============================================================ */
static int cmp_fields(const Student *a, const Student *b, int field, int order) {
    int result;
    switch (field) {
        case 1: /* 学号 */
            result = (a->id > b->id) - (a->id < b->id); // 刻意用比较而非相减，避免学号（最大1999999999）相减溢出
            break;
        case 2: /* 姓名 */
            result = strcmp(a->name, b->name);
            break;
        case 3: /* 年龄 */
            result = (a->age > b->age) - (a->age < b->age); // 同理
            break;
        case 4: /* 成绩 */
            if (a->score > b->score) {
                result = 1;
            } else if (a->score < b->score) {
                result = -1;
            } else {
                result = 0;
            }
            break;
        case 5: /* 学院 */
        default:
            result = strcmp(a->college, b->college);
            break;
    }
    /* 降序时反向 */
    return (order == 1) ? result : -result;
}

/* ---- 归并/快速排序递归时使用的模块级变量 ---- */
static int _sort_field;
static int _sort_order;

/* 供递归排序传递的比较包装 */
static int _cmp(const Student *a, const Student *b) {
    return cmp_fields(a, b, _sort_field, _sort_order);
}

/* ============================================================
 * 冒泡排序
 *
 * 重复遍历链表，逐对比较相邻结点，不符合排序规则则交换指针。
 * 时间复杂度 O(n²)，空间 O(1)。小数据量表现稳定，作为默认算法。
 * ============================================================ */
static void sort_bubble(Student **head, int field, int order) {
    int swapped;
    Student *pTmp;
    do {
        swapped = 0;
        Student **pp = head;
        while (*pp && (*pp)->next) {
            if (cmp_fields(*pp, (*pp)->next, field, order) > 0) {
                /* 交换两个相邻结点 */
                pTmp = *pp;
                *pp = (*pp)->next;
                pTmp->next = (*pp)->next;
                (*pp)->next = pTmp;
                swapped = 1;
            }
            pp = &(*pp)->next;
        }
    } while (swapped);
}

/* ============================================================
 * 选择排序
 *
 * 每轮从剩余未排序部分选出最小（或最大）结点，摘除后链入结果链表末尾。
 * 时间复杂度 O(n²)，交换次数少（n 次摘链 + n 次链接）。
 * ============================================================ */
static void sort_selection(Student **head, int field, int order) {
    Student *sorted_head = NULL;
    Student *sorted_tail = NULL;

    while (*head) {
        /* 在剩余链表中定位最值结点及其前驱 */
        Student **min_pp = head;
        for (Student **pp = head; *pp; ) {
            if (cmp_fields(*pp, *min_pp, field, order) < 0) {
                min_pp = pp;
            }
            pp = &(*pp)->next;
        }

        /* 摘除最值结点 */
        Student *min_node = *min_pp;
        *min_pp = min_node->next;
        min_node->next = NULL;

        /* 链入结果链表末尾 */
        if (!sorted_head) {
            sorted_head = min_node;
            sorted_tail = min_node;
        } else {
            sorted_tail->next = min_node;
            sorted_tail = min_node;
        }
    }
    *head = sorted_head;
}

/* ============================================================
 * 插入排序
 *
 * 逐个从原链表中取出结点，按排序规则插入到新链表的正确位置。
 * 时间复杂度 O(n²)，但对近乎有序的数据接近 O(n)。
 * ============================================================ */
static void sort_insertion(Student **head, int field, int order) {
    Student *sorted = NULL;
    Student *cur = *head;

    while (cur) {
        Student *next = cur->next;

        /* 在已排序链表中找插入位置 */
        Student **pp = &sorted;
        while ((*pp) && (cmp_fields(cur, *pp, field, order) > 0)) {
            pp = &(*pp)->next;
        }

        /* 插入 */
        cur->next = *pp;
        *pp = cur;

        cur = next;
    }
    *head = sorted;
}

/* ---- 归并排序辅助：快慢指针分割链表 ---- */
static void split_list(Student *head, Student **left, Student **right) {
    if (!head || !head->next) {
        *left = head;
        *right = NULL;
        return;
    }
    Student *slow = head;
    Student *fast = head->next;
    while (fast) {
        fast = fast->next;
        if (fast) {
            slow = slow->next;
            fast = fast->next;
        }
    }
    *left = head;
    *right = slow->next;
    slow->next = NULL;   /* 断开两半 */
}

/* ---- 归并排序辅助：合并两个有序链表 ---- */
static Student* merge_sorted(Student *a, Student *b) {
    Student dummy;
    Student *tail = &dummy;
    dummy.next = NULL;

    while (a && b) {
        if (_cmp(a, b) <= 0) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = a ? a : b;
    return dummy.next;
}

/* ---- 归并排序递归体 ---- */
static Student* sort_merge_rec(Student *head) {
    if ((!head) || (!head->next)) {
        return head;
    }

    Student *left, *right;
    split_list(head, &left, &right);

    left  = sort_merge_rec(left);
    right = sort_merge_rec(right);

    return merge_sorted(left, right);
}

/* ============================================================
 * 归并排序
 *
 * 分治法：递归分割链表 → 排序子链表 → 合并有序链表。
 * 时间复杂度 O(n log n)，数据量大时显著优于 O(n²) 算法。
 * 空间 O(log n)（递归栈），对链表无需额外数组空间。
 * ============================================================ */
static void sort_merge(Student **head, int field, int order) {
    _sort_field = field;
    _sort_order = order;
    *head = sort_merge_rec(*head);
}

/* 三路分区链表结点对（head + tail） */
struct PartitionList {
    Student *head;
    Student *tail;
};

/* ---- 快速排序辅助：三路分区 ---- */
static void partition_three_way(Student *pivot,
                                 struct PartitionList *less,
                                 struct PartitionList *equal,
                                 struct PartitionList *greater) {
    Student *cur = pivot->next;
    while (cur) {
        Student *next = cur->next;
        cur->next = NULL;
        int cmp = _cmp(cur, pivot);

        if (cmp < 0) {
            if (!less->head) {
                less->head = cur;
                less->tail = cur;
            } else {
                less->tail->next = cur;
                less->tail = cur;
            }
        } else if (cmp == 0) {
            if (!equal->head) {
                equal->head = cur;
                equal->tail = cur;
            } else {
                equal->tail->next = cur;
                equal->tail = cur;
            }
        } else {
            if (!greater->head) {
                greater->head = cur;
                greater->tail = cur;
            } else {
                greater->tail->next = cur;
                greater->tail = cur;
            }
        }
        cur = next;
    }
}

/* ---- 快速排序辅助：递归 ---- */
static Student* sort_quick_rec(Student *head) {
    if (!head || !head->next) return head;

    Student *pivot = head;
    struct PartitionList less = {NULL, NULL};
    struct PartitionList equal = {NULL, NULL};
    struct PartitionList greater = {NULL, NULL};

    partition_three_way(pivot, &less, &equal, &greater);

    /* pivot 自身链入 equal 链表头部 */
    pivot->next = equal.head;
    equal.head = pivot;
    if (!equal.tail) {
        equal.tail = pivot;
    }

    /* 递归排序 less 和 greater */
    less.head    = sort_quick_rec(less.head);
    greater.head = sort_quick_rec(greater.head);

    /* 拼接：less → equal → greater */
    Student *result;
    if (less.head) {
        result = less.head;
        /* 找到 less 尾 */
        Student *t = less.head;
        while (t->next) {
            t = t->next;
        }
        t->next = equal.head;
    } else {
        result = equal.head;
    }
    equal.tail->next = greater.head;

    return result;
}

/* ============================================================
 * 快速排序
 *
 * 分治法：选头结点为 pivot，三路分区后递归排序子链表再拼接。
 * 平均时间复杂度 O(n log n)，最坏 O(n²)（已有序时退化为冒泡）。
 * 空间 O(log n)（递归栈）。
 * ============================================================ */
static void sort_quick(Student **head, int field, int order) {
    _sort_field = field;
    _sort_order = order;
    *head = sort_quick_rec(*head);
}

/* ---- 梳排序辅助：按 gap 步进获取第 k 个后继结点 ---- */
static Student* node_at_offset(Student *node, int offset) {
    while (node && offset-- > 0) {
        node = node->next;
    }
    return node;
}

/* ============================================================
 * 梳排序
 *
 * 冒泡排序的改进版：不再只比较相邻元素，而是以递减的间隔 gap
 * 进行比较和交换。初始 gap = n / 1.3，每轮 gap /= 1.3，直到 gap=1
 * 且无交换为止。能快速消除远距离逆序（"乌龟"问题）。
 * 时间复杂度 O(n²) 最坏，但实践中通常远优于冒泡。
 * ============================================================ */
static void sort_comb(Student **head, int field, int order) {
    /* 统计链表长度 */
    int n = 0;

    for (Student *p = *head; p; p = p->next) {
        n++;
    }

    if (n < 2) {
        return;
    }

    int gap = n;
    int swapped;
    const double shrink = 1.3;

    do {
        /* 收缩间隔 */
        gap = (int)(gap / shrink);
        if (gap < 1) gap = 1;
        swapped = 0;

        Student **pp = head;
        while (*pp) {
            /* 找到 gap 步之后的结点 */
            Student *ahead = node_at_offset(*pp, gap);
            if (!ahead) {
                break;   /* 超出链表尾部 */
            }

            if (cmp_fields(*pp, ahead, field, order) > 0) {
                /* 交换两个结点：摘除 ahead 并插入到 *pp 前面 */
                /* 先找到 ahead 的前驱 */
                Student *prev_ahead = *pp;
                for (int i = 0; i < gap - 1; i++) {
                    prev_ahead = prev_ahead->next;
                }

                /* 摘除 ahead */
                prev_ahead->next = ahead->next;

                /* 插入 ahead 到 *pp 前面 */
                ahead->next = *pp;
                *pp = ahead;
                swapped = 1;

                /* 仍需推进 pp */
                pp = &ahead->next;
            } else {
                pp = &(*pp)->next;
            }
        }
    } while ((gap > 1) || (swapped));
}

/* ============================================================
 * 排序入口 — 选择算法、字段、升降序后执行排序
 *
 * 算法列表：
 *   1) 冒泡排序（默认，直接回车等同选 1）
 *   2) 选择排序
 *   3) 插入排序
 *   4) 归并排序（O(n log n)）
 *   5) 快速排序（O(n log n)）
 *   6) 梳排序（冒泡改进）
 * ============================================================ */
/* 交互式选择排序算法，返回 1~6 或 -1（用户取消） */
static int select_sort_algorithm(void) {
    printf("排序算法:\n");
    printf("  1) 冒泡排序（默认）  2) 选择排序\n");
    printf("  3) 插入排序          4) 归并排序\n");
    printf("  5) 快速排序          6) 梳排序\n");
    int algo = 1;  /* 默认冒泡 */
    char buf[INPUT_BUF_LEN];
    if (safe_get_string_allow_empty("请选择 (默认1): ", buf, sizeof(buf)) != 0) {
        return -1;
    }
    if (buf[0] != '\0') {
        char *endptr;
        long v = strtol(buf, &endptr, 10);
        if ((endptr == buf) || (*endptr != '\0') || (v < 1) || (v > 6)) {
            printf("[!] 输入无效，使用默认算法（冒泡排序）。\n");
        } else {
            algo = (int)v;
        }
    }
    return algo;
}

/* ============================================================
 * 排序入口 — 选择算法、字段、升降序后执行排序
 * ============================================================ */
void sort_students(Student **head) {
    if ((!*head) || (!(*head)->next)) {
        printf("(记录不足，无需排序)\n");
        return;
    }

    int algo = select_sort_algorithm();
    if (algo < 0) {
        return;
    }

    /* ---- 选择排序字段 ---- */
    int field;
    printf("\n排序字段:\n");
    printf("  1) 学号    2) 姓名    3) 年龄\n");
    printf("  4) 成绩    5) 学院\n");
    if (safe_get_int("请选择: ", 1, 5, &field) != 0) {
        return;
    }

    /* ---- 选择升降序 ---- */
    int order;
    printf("排序方式: 1) 升序  2) 降序\n");
    if (safe_get_int("请选择: ", 1, 2, &order) != 0) {
        return;
    }

    /* ---- 执行排序（计时） ---- */
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    switch (algo) {
        case 1: sort_bubble(head, field, order);    break;
        case 2: sort_selection(head, field, order); break;
        case 3: sort_insertion(head, field, order); break;
        case 4: sort_merge(head, field, order);     break;
        case 5: sort_quick(head, field, order);     break;
        case 6: sort_comb(head, field, order);      break;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) * 1000.0
                   + (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

    /* 先输出排序结果，再打印耗时 */
    display_all(*head);

    const char *algo_name[] = {"", "冒泡排序", "选择排序", "插入排序",
                               "归并排序", "快速排序", "梳排序"};
    printf("[OK] %s完成 (耗时 %.3f ms)。\n", algo_name[algo], elapsed);
}
