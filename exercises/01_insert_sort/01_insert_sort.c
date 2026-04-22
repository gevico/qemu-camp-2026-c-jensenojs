#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[20];
    int score;
} Student;

void swap(Student arr[], int i, int j) {
    Student tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = arr[i];
}

void insertion_sort(Student students[], int n) {
    if (n == 0 || n == 1) return;

    int i_idx = 0, i_score = students[i_idx].score;
    while(i_idx < n - 1) {
        int a = i_idx, b = a + 1, j_score = students[b].score;

        while(j_score > i_score) {
            swap(students, a, b);
            a--, b--;
            if(a < 0) {
                break;
            }
            i_score = students[a].score;
            j_score = students[b].score;
        }
        i_idx++;
    }
}

int main(void) {
    FILE *file;
    Student students[50];
    int n = 0;

    // 打开文件（从命令行参数获取文件名）
    file = fopen("01_students.txt", "r");
    if (!file) {
        printf("错误：无法打开文件 01_students.txt\n");
        return 1;
    }

    // 从文件读取学生信息
    while (n < 50 && fscanf(file, "%s %d", students[n].name, &students[n].score) == 2) {
        n++;
    }
    fclose(file);

    if (n == 0) {
        printf("文件中没有学生信息\n");
        return 1;
    }

    insertion_sort(students, n);

    printf("\n按成绩从高到低排序后的学生信息:\n");
    for (int i = 0; i < n; i++) {
        printf("%s %d\n", students[i].name, students[i].score);
    }

    return 0;
}

