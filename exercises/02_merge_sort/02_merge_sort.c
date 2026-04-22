#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STUDENTS 100
#define NAME_LEN 50

typedef struct {
    char name[NAME_LEN];
    int score;
} Student;

Student students[MAX_STUDENTS];
Student temp[MAX_STUDENTS];


void merge(int left, int mid, int right) {
    int l_idx = left, r_idx = mid + 1, t_idx = left;
    while(l_idx <= mid && r_idx <= right) {
        int ls = students[l_idx].score, rs = students[r_idx].score;
        if (ls >= rs) {
            temp[t_idx++] = students[l_idx++];
        } else {
            temp[t_idx++] = students[r_idx++];
        }
    }

    while (l_idx <= mid) {
        temp[t_idx++] = students[l_idx++];
    }
    while (r_idx <= right) {
        temp[t_idx++] = students[r_idx++];
    }

    for (int i = left; i <= right; i++) {
        students[i] = temp[i];
    }
}

void merge_sort(int left, int right) {
    if (right <= left) return;

    int mid = left + (right - left) / 2;

    merge_sort(left, mid);
    merge_sort(mid + 1, right);

    merge(left, mid, right);
}

int main(void) {
    FILE *file = fopen("02_students.txt", "r");
    if (!file) {
        printf("错误：无法打开文件 02_students.txt\n");
        return 1;
    }

    int n;
    fscanf(file, "%d", &n);

    if (n <= 0 || n > MAX_STUDENTS) {
        printf("学生人数无效：%d\n", n);
        fclose(file);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        fscanf(file, "%s %d", students[i].name, &students[i].score);
    }
    fclose(file);

    merge_sort(0, n - 1);

    printf("\n归并排序后按成绩从高到低排序的学生名单：\n");
    for (int i = 0; i < n; i++) {
        printf("%s %d\n", students[i].name, students[i].score);
    }

    return 0;
}
