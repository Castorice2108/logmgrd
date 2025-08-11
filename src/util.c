#include "util.h"
#include <string.h>

/**
 * 去除字符串末尾的换行、回车和空格
 */
void trim(char *s) {
    size_t l = strlen(s);
    while (l > 0 && (s[l-1] == '\n' || s[l-1] == '\r' || s[l-1] == ' ')) s[--l] = 0;
}

/**
 * 判断字符串line是否以prefix开头
 * 返回1是，返回0否
 */
int str_starts(const char *line, const char *prefix) {
    return strncmp(line, prefix, strlen(prefix)) == 0;
}