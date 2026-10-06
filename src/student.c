/*
 * student.c — 链表操作模块
 *
 * 实现学生信息的增、删、改、查、排序、统计等核心业务逻辑。
 * 数据存储于动态单向链表中，所有修改操作自动触发文件保存。
 */

#include "student.h"
#include "hash.h"
#include <stdio.h>

static int g_data_dirty = 0;   /* 是否有未保存的修改 */

void mark_data_dirty(void) { g_data_dirty = 1; }
void mark_data_clean(void) { g_data_dirty = 0; }
int  is_data_dirty(void)   { return g_data_dirty; }

/*
 * 创建并初始化一个新结点。
 * 分配内存后将 next 置 NULL，调用者负责将其链入链表。
 * 返回 NULL 表示内存不足。
 */
Student* create_node(void) {
    Student *node = (Student*)malloc(sizeof(Student));
    if (!node) {
        printf("[!] 内存分配失败。\n");
        return NULL;
    }
    node->next = NULL;
    node->hash_next = NULL;
    return node;
}

/*
 * 交互式输入性别。
 * allow_empty=0 时必须输入"男"或"女"；allow_empty=1 时空输入直接返回。
 * 返回 0 成功，-1 EOF/错误。
 */
static int input_gender(const char *prompt, char *gender_buf, int allow_empty) {
    while (1) {
        int rc = allow_empty
            ? safe_get_string_allow_empty(prompt, gender_buf, GENDER_LEN)
            : safe_get_string(prompt, gender_buf, GENDER_LEN);
        if (rc != 0) {
            return -1;
        }
        if (allow_empty && gender_buf[0] == '\0') {
            return 0;
        }
        if (strcmp(gender_buf, "男") == 0 || strcmp(gender_buf, "女") == 0) {
            return 0;
        }
        printf("[!] 性别只能输入\"男\"或\"女\"。\n");
    }
}

/*
 * 交互式添加一名学生。
 * 依次输入学号、姓名、性别、年龄、成绩，均做合法性校验。
 * 学号唯一性由 search_by_id 保证，重复则拒绝添加并释放已分配内存。
 * 添加成功后自动保存到文件。
 * 返回 0 成功，-1 失败（用户取消或输入异常）。
 */
int add_student(Student **head) {
    Student *node = create_node();
    if (!node) {
        return -1;
    }

    /* --- 学号：范围 + 唯一性 --- */
    if (safe_get_int("学号 (1~1999999999): ", MIN_ID, MAX_ID, &node->id) != 0) {
        free(node);
        return -1;
    }
    if (search_by_id(node->id)) {
        printf("[!] 学号 %d 已存在，添加失败。\n", node->id);
        free(node);
        return -1;
    }

    /* --- 姓名 --- */
    if (safe_get_string("姓名: ", node->name, NAME_LEN) != 0) {
        free(node);
        return -1;
    }

    /* --- 性别：仅接受"男"或"女" --- */
    if (input_gender("性别 (男/女): ", node->gender, 0) != 0) {
        free(node);
        return -1;
    }

    /* --- 年龄 --- */
    if (safe_get_int("年龄 (0~150): ", MIN_AGE, MAX_AGE, &node->age) != 0) {
        free(node);
        return -1;
    }

    /* --- 成绩 --- */
    if (safe_get_float("成绩 (0~100): ", MIN_SCORE, MAX_SCORE, &node->score) != 0) {
        free(node);
        return -1;
    }

    /* --- 学院（米哈游游戏名） --- */
    if (safe_get_string("学院: ", node->college, COLLEGE_LEN) != 0) {
        free(node);
        return -1;
    }

    /* 头插法链入链表 + 哈希表 */
    node->next = *head;
    *head = node;
    hash_insert(node);
    printf("[OK] 学生 %s (学号: %d) 添加成功。\n", node->name, node->id);
    mark_data_dirty();
    return 0;
}

/*
 * 以表格形式显示所有学生信息。
 * 使用 print_field 保证中英文混合时列对齐。
 * 链表为空时输出提示信息。
 */
void display_all(const Student *head) {
    if (!head) {
        printf("(暂无学生记录)\n\n");
        return;
    }
    printf("\n");

    /* 表头 */
    print_field("学号", COL_ID, 0);
    printf(" ");
    print_field("姓名", COL_NAME, 1);
    printf(" ");
    print_field("性别", COL_GENDER, 1);
    printf(" ");
    print_field("年龄", COL_AGE, 0);
    printf(" ");
    print_field("成绩", COL_SCORE, 0);
    printf(" ");
    print_field("学院", COL_COLLEGE, 1);
    printf("\n");

    /* 分隔线 */
    int total = COL_ID+1 + COL_NAME+1 + COL_GENDER+1 + COL_AGE+1
              + COL_SCORE+1 + COL_COLLEGE;
    for (int i = 0; i < total; i++) {
        putchar('-');
    }
    putchar('\n');

    /* 逐行输出 */
    for (const Student *p = head; p; p = p->next) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", p->id);
        print_field(buf, COL_ID, 0);
        printf(" ");
        print_field(p->name, COL_NAME, 1);
        printf(" ");
        print_field(p->gender, COL_GENDER, 1);
        printf(" ");
        snprintf(buf, sizeof(buf), "%d", p->age);
        print_field(buf, COL_AGE, 0);
        printf(" ");
        snprintf(buf, sizeof(buf), "%.2f", p->score);
        print_field(buf, COL_SCORE, 0);
        printf(" ");
        print_field(p->college, COL_COLLEGE, 1);
        printf("\n");
    }
    printf("\n");
}

/*
 * 按学号查找学生。
 * 遍历链表，匹配则返回结点指针，否则返回 NULL。
 */
Student* search_by_id(int id) {
    return hash_find(id);
}

/*
 * 按学号删除学生。
 * 输入学号，找到后从链表中摘除结点并释放内存。
 * 删除后自动保存文件。
 * 返回 0 成功，-1 失败（列表为空或未找到）。
 */
int delete_student(Student **head) {
    if (!*head) {
        printf("(暂无学生记录，无法删除)\n");
        return -1;
    }

    int id;
    if (safe_get_int("请输入要删除的学号: ", MIN_ID, MAX_ID, &id) != 0) {
        return -1;
    }

    /* 查找目标结点及其前驱 */
    Student *prev = NULL, *cur = *head;
    while (cur && cur->id != id) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("[!] 未找到学号为 %d 的学生。\n", id);
        return -1;
    }

    /* 摘除结点 */
    if (prev) {
        prev->next = cur->next;
    } else {
        *head = cur->next;   /* 删除的是头结点 */
    }

    printf("[OK] 学生 %s (学号: %d) 已删除。\n", cur->name, cur->id);
    hash_remove(cur->id);
    free(cur);
    mark_data_dirty();
    return 0;
}

/*
 * 可选的整数字段修改：空输入跳过，否则解析并校验范围。
 * 返回 0 成功/跳过，-1 EOF。
 */
struct IntRange { int min, max; };

static int modify_optional_int(const char *prompt, char *buf, size_t buf_size,
                                struct IntRange range, int *dest) {
    if (safe_get_string_allow_empty(prompt, buf, buf_size) != 0) {
        return -1;
    }

    if (buf[0] == '\0') {
        return 0;
    }

    char *endptr;
    long val = strtol(buf, &endptr, 10);
    if (endptr != buf && *endptr == '\0' && val >= range.min && val <= range.max) {
        *dest = (int)val;
    } else {
        printf("[!] 输入无效，保留原值。\n");
    }
    return 0;
}

/*
 * 可选的浮点字段修改：空输入跳过，否则解析并校验范围。
 */
struct FloatRange { float min, max; };

static int modify_optional_float(const char *prompt, char *buf, size_t buf_size,
                                  struct FloatRange range, float *dest) {
    if (safe_get_string_allow_empty(prompt, buf, buf_size) != 0) {
        return -1;
    }

    if (buf[0] == '\0') {
        return 0;
    }

    char *endptr;
    float val = strtof(buf, &endptr);
    if (endptr != buf && *endptr == '\0' && val >= range.min && val <= range.max) {
        *dest = val;
    } else {
        printf("[!] 输入无效，保留原值。\n");
    }
    return 0;
}

/*
 * 按学号修改学生信息。
 * 用户可选择性地修改姓名、性别、年龄、成绩；
 * 直接按回车表示保留原值（不修改）。
 * 修改后自动保存文件。
 */
int modify_student(Student *head) {
    if (!head) {
        printf("(暂无学生记录，无法修改)\n");
        return -1;
    }

    int id;
    if (safe_get_int("请输入要修改的学号: ", MIN_ID, MAX_ID, &id) != 0) {
        return -1;
    }

    Student *s = search_by_id(id);
    if (!s) {
        printf("[!] 未找到学号为 %d 的学生。\n", id);
        return -1;
    }

    printf("(直接按回车表示不修改该字段)\n");

    char buf[INPUT_BUF_LEN];

    /* 修改姓名：空输入即跳过 */
    if (safe_get_string_allow_empty("新姓名: ", buf, sizeof(buf)) != 0) {
        return -1;
    }

    if (buf[0] != '\0') {
        strncpy(s->name, buf, NAME_LEN - 1);
        s->name[NAME_LEN - 1] = '\0';
    }

    /* 修改性别：空输入跳过，否则校验"男"/"女" */
    if (input_gender("新性别 (男/女): ", buf, 1) != 0) {
        return -1;
    }

    if (buf[0] != '\0') {
        strncpy(s->gender, buf, GENDER_LEN - 1);
        s->gender[GENDER_LEN - 1] = '\0';
    }

    /* 修改年龄：空输入跳过，否则校验范围 */
    if (modify_optional_int("新年龄 (0~150): ", buf, sizeof(buf),
                             (struct IntRange){MIN_AGE, MAX_AGE}, &s->age) != 0) {
        return -1;
    }
    /* 修改成绩：空输入跳过，否则校验范围 */
    if (modify_optional_float("新成绩 (0~100): ", buf, sizeof(buf),
                               (struct FloatRange){MIN_SCORE, MAX_SCORE}, &s->score) != 0) {
        return -1;
    }

    /* 修改学院：空输入跳过 */
    if (safe_get_string_allow_empty("新学院: ", buf, sizeof(buf)) != 0) {
        return -1;
    }

    if (buf[0] != '\0') {
        strncpy(s->college, buf, COLLEGE_LEN - 1);
        s->college[COLLEGE_LEN - 1] = '\0';
    }

    printf("[OK] 学生信息已更新。\n");

    mark_data_dirty();

    return 0;
}

#define LW 18  /* 标签列宽（显示列，含冒号） */

static void statistics_headcount(const Student *head) {
    if (!head) {
        printf("[!] (暂无学生记录，无法统计)\n");
        return;
    }

    int count = 0, male = 0, female = 0;

    for (const Student *p = head; p; p = p->next) {
        count++;
        if (strcmp(p->gender, "男") == 0) {
            male++;
        } else {
            female++;
        }
    }

    char buf[64];

    printf("\n========== 人数统计 ==========\n");
    snprintf(buf, sizeof(buf), "%d", count);
    print_field("总人数:", LW, 1);
    printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%d (%.1f%%)", male, male * 100.0f / count);
    print_field("男生人数:", LW, 1);
    printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%d (%.1f%%)", female, female * 100.0f / count);
    print_field("女生人数:", LW, 1);
    printf("%s\n", buf);
}

static void statistics_score(const Student *head) {
    if (!head) {
        printf("[!] (暂无学生记录，无法统计)\n");
        return;
    }

    float total = 0.0f, max_score = -1.0f, min_score = 101.0f;
    int fail_count = 0;
    int count = 0;

    for (const Student *p = head; p; p = p->next) {
        total += p->score;
        count++;

        if (p->score > max_score) {
            max_score = p->score;
        }

        if (p->score < min_score) {
            min_score = p->score;
        }

        if (p->score < 60.0f) {
            fail_count++;
        }
    }

    char buf[64];
    printf("\n========== 成绩统计 ==========\n");
    snprintf(buf, sizeof(buf), "%.2f", total / count);
    print_field("平均分:", LW, 1);
    printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%.2f", max_score);
    print_field("最高分:", LW, 1);
    printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%.2f", min_score);
    print_field("最低分:", LW, 1);
    printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%d (<60分)", fail_count);
    print_field("不及格人数:", LW, 1);
    printf("%s\n", buf);
}

static void statistics_age(const Student *head) {
    if (!head) {
        printf("[!] (暂无学生记录，无法统计)\n");
        return;
    }

    int age_udr_18 = 0, age_btw_18_22 = 0, age_btw_22_26 = 0, age_over_26 = 0;
    int count = 0;

    for (const Student *p = head; p; p = p->next) {
        count++;

        if (p->age < 18) {
            age_udr_18++;
        } else if (p->age < 22) {
            age_btw_18_22++;
        } else if (p->age < 26) {
            age_btw_22_26++;
        } else {
            age_over_26++;
        }
    }

    char buf[64];

    printf("\n========== 年龄分布 ==========\n");
    print_field("年龄段", LW, 1);
    print_field("人数（百分比）", LW, 1);
    printf("\n——————————————————————————————\n");

    snprintf(buf, sizeof(buf), "%d (%.1f%%)", age_udr_18, age_udr_18 * 100.0f / count);
    print_field("≤18岁:", LW, 1); printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%d (%.1f%%)", age_btw_18_22, age_btw_18_22 * 100.0f / count);
    print_field("19-22岁:", LW, 1); printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%d (%.1f%%)", age_btw_22_26, age_btw_22_26 * 100.0f / count);
    print_field("23-25岁:", LW, 1); printf("%s\n", buf);
    snprintf(buf, sizeof(buf), "%d (%.1f%%)", age_over_26, age_over_26 * 100.0f / count);
    print_field("≥26岁:", LW, 1); printf("%s\n", buf);
}

static void statistics_college(const Student *head) {
    if (!head) {
        printf("[!] (暂无学生记录，无法统计)\n");
        return;
    }

    struct CollegeStat {
        char name[COLLEGE_LEN];
        int  cnt;
    } cs[32];

    int count = 0;
    int cs_count = 0;

    for (const Student *p = head; p; p = p->next) {
        count++;

        int found = 0;
        for (int i = 0; i < cs_count; i++) {
            if (strcmp(cs[i].name, p->college) == 0) {
                cs[i].cnt++;
                found = 1;
                break;
            }
        }

        if ((!found) && (cs_count < 32)) {
            strncpy(cs[cs_count].name, p->college, COLLEGE_LEN - 1);
            cs[cs_count].name[COLLEGE_LEN - 1] = '\0';
            cs[cs_count].cnt = 1;
            cs_count++;
        }
    }

    char buf[64];

    /* --- 学院分布 --- */
    printf("\n========== 学院分布 ==========\n");
    print_field("学院名称", LW, 1);
    print_field("人数（百分比）", LW, 1);
    printf("\n——————————————————————————————\n");

    for (int i = 0; i < cs_count; i++) {
        snprintf(buf, sizeof(buf), "%d (%.1f%%)", cs[i].cnt, cs[i].cnt * 100.0f / count);
        print_field(cs[i].name, LW, 1);
        printf("%s\n", buf);
    }
}

#undef LW
#undef VW

/*
 * 显示统计信息：总人数、男女比例、成绩统计、年龄分布、学院分布。
 * 使用 print_field 保证中英文混合对齐。
 */
void show_statistics(const Student *head) {
    statistics_headcount(head);

    statistics_score(head);

    statistics_age(head);

    statistics_college(head);
}

/*
 * 释放整个链表的内存。
 * 遍历所有结点逐一 free，最后将头指针置 NULL，
 * 防止悬空指针和重复释放。
 */
void free_list(Student **head) {
    Student *cur = *head;
    while (cur) {
        Student *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    *head = NULL;
    hash_destroy();
}
