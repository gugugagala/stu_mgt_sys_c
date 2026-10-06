/*
 * fileio.c — 文件持久化模块
 *
 * 实现数据从文件加载和保存到文件的功能。
 *
 * 加载策略：逐行解析，严格校验字段数量和数值范围，
 *           损坏行自动跳过并警告，不影响其余数据。
 *
 * 保存策略（原子写入）：
 *   1. 备份原文件 → students.bak
 *   2. 写入临时文件 → students.tmp
 *   3. 临时文件重命名为正式文件 → students.txt
 *   4. 任何步骤失败均回滚，保证原数据不被破坏
 */

#include "student.h"
#include "hash.h"

/* 解析出的记录临时结构，用于在 parse_record_line 和 append_new_node 间传递 */
struct ParsedRecord {
    int   id;
    char  name[NAME_LEN];
    char  gender[GENDER_LEN];
    int   age;
    float score;
    char  college[COLLEGE_LEN];
};

/* 前向声明 */
static int  parse_record_line(const char *line, int line_no,
                               struct ParsedRecord *rec);
static Student* append_new_node(Student **head, Student **tail,
                                 const struct ParsedRecord *rec);
static void rollback_on_write_failure(int has_backup);

/*
 * 从 DATA_FILE 加载学生数据。
 * 采用 fgets + sscanf 逐行解析，每行必须恰好包含 5 个字段。
 * 使用尾插法保持文件原始顺序。
 * 对每条记录二次校验学号、年龄、成绩的合法范围，
 * 不合规的行将被跳过并输出警告。
 *
 * 返回 0 成功（含文件不存在的情况），-1 内存分配失败。
 */
int load_from_file(Student **head) {
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp) {
        /* 文件不存在视为首次运行，静默返回 */
        return 0;
    }

    char line[512];      /* 单行缓冲区，足够容纳一条完整记录 */
    int line_no = 0;     /* 当前行号，用于错误定位 */
    int loaded = 0;      /* 成功加载的记录数 */
    Student *tail = NULL; /* 尾指针，用于保持文件顺序 */

    while (fgets(line, sizeof(line), fp)) {
        line_no++;

        /* 去除末尾换行符 */
        size_t len = strlen(line);
        if ((len > 0 )&& (line[len - 1] == '\n')) {
            line[len - 1] = '\0';
        }
        if (line[0] == '\0') {
            continue;   /* 跳过空行 */
        }

        struct ParsedRecord rec;

        if (parse_record_line(line, line_no, &rec) != 0) {
            continue;
        }

        if (!append_new_node(head, &tail, &rec)) {
            fclose(fp);
            return -1;
        }
        loaded++;
    }
    fclose(fp);

    if (loaded > 0) {
        printf("[OK] 已从 %s 加载 %d 条记录。\n", DATA_FILE, loaded);
    }
    return 0;
}

/*
 * 解析并校验一行记录。
 * 对 sscanf 字段数、年龄/成绩/学号范围、学号唯一性逐一检查。
 * 返回 0 表示解析成功，-1 表示该行应跳过（已输出警告）。
 */
static int parse_record_line(const char *line, int line_no,
                              struct ParsedRecord *rec) {
    int matched = sscanf(line, "%d %31s %7s %d %f %31s",
                         &rec->id, rec->name, rec->gender,
                         &rec->age, &rec->score, rec->college);
    if (matched != 6) {
        printf("[!] 第 %d 行字段数不正确，已跳过。\n", line_no);
        return -1;
    }
    if (rec->age < MIN_AGE || rec->age > MAX_AGE) {
        printf("[!] 第 %d 行年龄 %d 超出范围，已跳过。\n", line_no, rec->age);
        return -1;
    }
    if (rec->score < MIN_SCORE || rec->score > MAX_SCORE) {
        printf("[!] 第 %d 行成绩 %.2f 超出范围，已跳过。\n", line_no, rec->score);
        return -1;
    }
    if (rec->id < MIN_ID || rec->id > MAX_ID) {
        printf("[!] 第 %d 行学号 %d 无效，已跳过。\n", line_no, rec->id);
        return -1;
    }
    if (search_by_id(rec->id)) {
        printf("[!] 第 %d 行学号 %d 重复，已跳过。\n", line_no, rec->id);
        return -1;
    }
    return 0;
}

/*
 * 创建新结点、填充数据并以尾插法链入链表。
 * 返回新结点指针，失败返回 NULL（内存不足）。
 */
static Student* append_new_node(Student **head, Student **tail,
                                 const struct ParsedRecord *rec) {
    Student *node = create_node();
    if (!node) {
        return NULL;
    }

    node->id = rec->id;
    strncpy(node->name, rec->name, NAME_LEN - 1);
    node->name[NAME_LEN - 1] = '\0';
    strncpy(node->gender, rec->gender, GENDER_LEN - 1);
    node->gender[GENDER_LEN - 1] = '\0';
    node->age = rec->age;
    node->score = rec->score;
    strncpy(node->college, rec->college, COLLEGE_LEN - 1);
    node->college[COLLEGE_LEN - 1] = '\0';

    if (*tail) {
        (*tail)->next = node;
    } else {
        *head = node;
    }
    *tail = node;
    hash_insert(node);
    return node;
}

/*
 * 写入失败时的回滚操作：删除临时文件，从备份恢复原文件。
 */
static void rollback_on_write_failure(int has_backup) {
    printf("[!] 写入临时文件失败，数据未保存。\n");
    remove(TEMP_FILE);
    if (has_backup) {
        if (rename(BACKUP_FILE, DATA_FILE) != 0) {
            printf("[!] 恢复备份失败！请手动检查 %s。\n", BACKUP_FILE);
        }
    }
}

/*
 * 将链表数据原子写入 DATA_FILE。
 *
 * 原子写入流程：
 *   1. 若原文件存在，重命名为备份文件 (.bak)
 *   2. 将所有数据写入临时文件 (.tmp)
 *   3. 临时文件关闭成功 → 重命名为正式文件 (.txt)
 *   4. 删除备份文件
 *   5. 任一步骤失败 → 删除临时文件，从备份恢复原文件
 *
 * 返回 0 成功，-1 保存失败（原数据已被保护）。
 */
int save_to_file(const Student *head) {
    FILE *tmp = NULL;

    /* 步骤 1：备份原文件 */
    FILE *orig = fopen(DATA_FILE, "r");
    if (orig) {
        fclose(orig);
        remove(BACKUP_FILE);                          /* 删除旧备份 */
        if (rename(DATA_FILE, BACKUP_FILE) != 0) {
            printf("[!] 备份原文件失败，取消保存以防止数据丢失。\n");
            return -1;
        }
    }

    /* 步骤 2：写入临时文件 */
    tmp = fopen(TEMP_FILE, "w");
    if (!tmp) {
        printf("[!] 无法创建临时文件 %s。\n", TEMP_FILE);
        if (orig) {
            rename(BACKUP_FILE, DATA_FILE);    /* 恢复原文件 */
        }

        return -1;
    }

    int write_err = 0;
    for (const Student *p = head; p; p = p->next) {
        if (fprintf(tmp, "%d %s %s %d %.2f %s\n",
                    p->id, p->name, p->gender, p->age, p->score, p->college) < 0) {
            write_err = 1;
            break;
        }
    }

    if (fclose(tmp) != 0) {
        write_err = 1;
    }

    /* 步骤 3：验证并替换 */
    if (write_err) {
        rollback_on_write_failure(orig != NULL);
        return -1;
    }

    if (rename(TEMP_FILE, DATA_FILE) != 0) {
        printf("[!] 替换正式文件失败，数据保存在 %s 中。\n", TEMP_FILE);
        remove(BACKUP_FILE);   /* 清理残留备份 */
        return -1;
    }

    /* 步骤 4：清理备份 */
    remove(BACKUP_FILE);

    mark_data_clean();
    return 0;
}
