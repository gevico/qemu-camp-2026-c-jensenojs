#include <stdio.h>

void my_strcpy(char *dest, const char *src) {
    while (*src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int main() {
    char source[] = "Hello, World! This is a test string for my_strcpy function.\n";
    char destination[101];

    my_strcpy(destination, source);

    printf("拷贝后的字符串: %s", destination);

    return 0;
}
