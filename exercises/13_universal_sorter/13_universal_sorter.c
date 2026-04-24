#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*CompareFunc)(const void *, const void *);

/* int 比较器：qsort 传进来的是“数组元素的地址”，不是元素本身。
   所以这里要先把 a/b 当成 int*，再解引用成 int 值。*/
int compareInt(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

/* float 比较器：同样先把 a/b 当成 float* 再取值；
   返回 1 / -1 / 0，比直接返回差值更稳妥。 */
int compareFloat(const void *a, const void *b) {
    float diff = (*(float*)a - *(float*)b);
    return (diff > 0) ? 1 : ((diff < 0) ? -1 : 0);
}

/* 字符串比较器：如果数组里存的是 char*，那么每个“元素”本身就是一个指针。
   qsort 传进来的是“这个元素的地址”，所以 a/b 的类型要读成 char**。
   读法是：先把 a 当成 char**，再解引用一次，拿到真正的 char* 字符串。 */
int compareString(const void *a, const void *b) {
    return strcmp(*(char**)a, *(char**)b);
}

void sort(void *array, size_t n, size_t size, CompareFunc compare) {
    qsort(array, n, size, compare);
}

void processFile(const char *filename) {
    FILE *fin = fopen(filename, "r");
    if (!fin) {
        printf("错误: 无法打开文件 %s\n", filename);
        return;
    }

    int choice, n;
    if (fscanf(fin, "%d", &choice) != 1 || fscanf(fin, "%d", &n) != 1) {
        printf("错误: 文件 %s 格式不正确\n", filename);
        fclose(fin);
        return;
    }

    if (n > 20) n = 20;  // 最多支持20个元素

    printf("=== 处理数据来自: %s ===\n", filename);

    switch (choice) {
        case 1: {
            int data[20];
            for (int i = 0; i < n; i++) {
                fscanf(fin, "%d", &data[i]);
            }
            sort(data, n, sizeof(int), compareInt);
            for (int i = 0; i < n; i++) {
                printf("%d%s", data[i], (i == n - 1) ? "\n" : " ");
            }
            break;
        }
        case 2: {
            float data[20];
            for (int i = 0; i < n; i++) {
                fscanf(fin, "%f", &data[i]);
            }
            sort(data, n, sizeof(float), compareFloat);
            for (int i = 0; i < n; i++) {
                printf("%.2f%s", data[i], (i == n - 1) ? "\n" : " ");
            }
            break;
        }
        default: {
            char *data[20];
            char buffer[20][32];
            for (int i = 0; i < n; i++) {
                fscanf(fin, "%31s", buffer[i]);
                /* buffer[i] 作为每个字符串的存储区，data[i] 指向它 */
                data[i] = buffer[i];
            }
            sort(data, n, sizeof(char *), compareString);
            for (int i = 0; i < n; i++) {
                printf("%s%s", data[i], (i == n - 1) ? "\n" : " ");
            }
            break;
        }
    }

    fclose(fin);
}

int main() {
    processFile("int_sort.txt");
    processFile("float_sort.txt");

    return 0;
}
