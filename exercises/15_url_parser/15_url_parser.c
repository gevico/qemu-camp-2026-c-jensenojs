#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * URL参数解析器
 * 输入：包含http/https超链接的字符串
 * 输出：解析出所有的key-value键值对，每行显示一个
 */

int parse_url(const char* url) {
    int err = 0;
    const char *query = strchr(url, '?');
    if (query == NULL || *(query + 1) == '\0') {
        printf("没有找到参数\n");
        return 0;
    }

    char buf[256];
    strncpy(buf, query + 1, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *pair = strtok(buf, "&");
    while (pair != NULL) {
        char *eq = strchr(pair, '=');
        if (eq != NULL) {
            *eq = '\0';
            printf("key = %s, value = %s\n", pair, eq + 1);
        }
        pair = strtok(NULL, "&");
    }

exit:
    return err;
}

int main() {
    const char* test_url = "https://cn.bing.com/search?name=John&age=30&city=New+York";

    printf("Parsing URL: %s\n", test_url);
    printf("Parameters:\n");

    parse_url(test_url);

    return 0;
}
