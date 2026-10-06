/*
 * main.c — 程序入口与主控流程
 *
 * 负责：解析命令行参数（预留），加载数据，进入主菜单循环，
 *       调度各功能模块，并在退出前执行保存与内存释放。
 */

#include "student.h"
#include "sort.h"

/*
 * 以表格行形式显示单个学生的信息（含表头和分隔线）。
 */
static void print_student_table_row(const Student *s) {
    printf("\n");
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

    int total = (COL_ID + 1) + (COL_NAME + 1) + (COL_GENDER + 1) + (COL_AGE + 1)
              + (COL_SCORE + 1) + COL_COLLEGE;
    for (int i = 0; i < total; i++) {
        putchar('-');
    }
    putchar('\n');

    char buf[64];
    snprintf(buf, sizeof(buf), "%d", s->id);
    print_field(buf, COL_ID, 0);
    printf(" ");
    print_field(s->name, COL_NAME, 1);
    printf(" ");
    print_field(s->gender, COL_GENDER, 1);
    printf(" ");
    snprintf(buf, sizeof(buf), "%d", s->age);
    print_field(buf, COL_AGE, 0);
    printf(" ");
    snprintf(buf, sizeof(buf), "%.2f", s->score);
    print_field(buf, COL_SCORE, 0);
    printf(" ");
    print_field(s->college, COL_COLLEGE, 1);
    printf("\n");
}

static void menu_search_student(void) {
    int id;
    if (safe_get_int("请输入要查找的学号: ", MIN_ID, MAX_ID, &id) != 0) {
        return;
    }

    Student *s = search_by_id(id);

    if (s) {
        print_student_table_row(s);
    } else {
        printf("[!] 未找到学号为 %d 的学生。\n", id);
    }
}

static void menu_save_stus_info(Student **head) {
    if (save_to_file(*head) == 0) {
        printf("[OK] 数据已手动保存。\n");
    } else {
        printf("[!] 保存失败。\n");
    }
}

static void menu_quit(Student **head) {
    if (is_data_dirty()) {
        printf("\n有未保存的修改。\n");
        char buf[8];
        if (safe_get_string("是否保存后退出? (y/n): ", buf, sizeof(buf)) == 0
            && (buf[0] == 'y' || buf[0] == 'Y')) {
            if (save_to_file(*head) == 0)
                printf("[OK] 数据已保存。\n");
        }
    }
    printf("正在退出...\n");
}

/*
 * 执行用户选择的菜单操作。
 * 返回 0 继续运行，-1 退出程序。
 */
static int do_menu_action(Student **head, int choice) {
    switch (choice) {
        case 1:
            add_student(head);
            break;
        case 2:
            display_all(*head);
            break;
        case 3: {
            menu_search_student();
            break;
        }
        case 4:
            delete_student(head);
            break;
        case 5:
            modify_student(*head);
            break;
        case 6:
            sort_students(head);
            break;
        case 7:
            show_statistics(*head);
            break;
        case 8:
            menu_save_stus_info(head);
            break;
        case 0:
            menu_quit(head);
            return -1;
        default: break;
    }
    return 0;
}

int main(void) {
    Student *head = NULL;
    int choice;

    printf("+-----------------------------------------+\n");
    printf("|       学生信息管理系统 启动中...        |\n");
    printf("+-----------------------------------------+\n");

    if (load_from_file(&head) != 0) {
        printf("[!] 数据加载异常，将以空数据库启动。\n");
    }

    do {
        print_menu();
        if (safe_get_int("请选择操作: ", 0, 8, &choice) != 0) {
            printf("[!] 输入错误，请重试。\n");
            continue;
        }
    } while (do_menu_action(&head, choice) == 0);

    free_list(&head);
    printf("系统已安全退出。\n");
    return 0;
}
