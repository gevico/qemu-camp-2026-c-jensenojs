#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

#include "common.h"

#define MAX_INPUT 1024
#define MAX_ARGS 64

// ======================
// 自定义命令系统
// ======================

// 函数指针类型定义
typedef int (*cmd_func_0_t)(void);
typedef int (*cmd_func_1_t)(const char*);
typedef int (*cmd_func_2_t)(const char*, const char*);

// 命令结构体
typedef struct {
    const char *name;             // 命令名，如 "myfile"
    int is_arg_required;          // 是否需要参数：1 需要，0 不需要
    union {
        cmd_func_0_t func_0;      // 无参数函数
        cmd_func_1_t func_1;      // 带一个 const char* 参数的函数
        cmd_func_2_t func_2;
    } func;
} Command;

// 命令表：手动注册所有支持的外部命令
Command commands[] = {
    {"myfile", 1, .func.func_1 = __cmd_myfile},   // 需要 1 个参数
    {"mysed",  2, .func.func_2 = __cmd_mysed},    // 需要 2 个参数
    {"mytrans", 1, .func.func_1 = __cmd_mytrans}, // 需要 1 个参数
    {"mywc", 1, .func.func_1 = __cmd_mywc},       // 需要 1 个参数
    {NULL, 0, .func.func_0 = NULL}                // 结束标记
};

// ======================
// 原有内置命令 & 工具函数
// ======================

void execute_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "mybash: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("mybash");
    }
  }
}

void execute_exit() { exit(0); }

static char *dup_token(const char *s) {
  size_t len = strlen(s);
  char *copy = malloc(len + 1);
  if (copy == NULL)
    return NULL;
  memcpy(copy, s, len + 1);
  return copy;
}

static void free_args(char **args) {
  for (int i = 0; args[i] != NULL; i++) {
    free(args[i]);
    args[i] = NULL;
  }
}

int is_builtin_command(char **args) {
  if (args[0] == NULL)
    return 0;

  if (strcmp(args[0], "cd") == 0) {
    execute_cd(args);
    return 1;
  }
  if (strcmp(args[0], "exit") == 0) {
    execute_exit();
    return 1;
  }

  return 0;
}

int parse_input(char *input, char **args) {
  int i = 0;
  int in_quotes = 0;
  char quote_char = '\0';
  char *buf = input;
  char arg_buf[MAX_INPUT];  // 临时存储当前正在解析的参数
  int arg_buf_idx = 0;

  memset(arg_buf, 0, sizeof(arg_buf));

  while (*buf != '\0' && i < MAX_ARGS - 1) {
      char c = *buf;

      if (in_quotes) {
        if (c == quote_char) {
          in_quotes = 0;
        } else {
          arg_buf[arg_buf_idx++] = c;
        }
      } else {
        if (c == '"' || c == '\'') {
          in_quotes = 1;
          quote_char = c;
        } else if (isspace((unsigned char)c)) {
          if (arg_buf_idx > 0) {
            arg_buf[arg_buf_idx] = '\0';
            args[i++] = dup_token(arg_buf);
            arg_buf_idx = 0;
            memset(arg_buf, 0, sizeof(arg_buf));
          }
        } else {
          arg_buf[arg_buf_idx++] = c;
        }
      }

      buf++;
  }

  // 处理最后一个参数（循环结束后可能还有未加入的）
  if (arg_buf_idx > 0) {
      arg_buf[arg_buf_idx] = '\0';
      args[i++] = dup_token(arg_buf);
  }

  args[i] = NULL;  // exec-style NULL结尾
  return i;
}

// ======================
// 主函数：命令循环 & 自定义命令分发
// ======================

int main(int argc, char *argv[]) {
  char input[MAX_INPUT];
  char *args[MAX_ARGS];

  if (argc > 1) {
    // 从文件读取命令
    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (!file) {
      printf("mybash: cannot open file: %s\n", filename);
      return 1;
    }

    printf("mybash: reading commands from file '%s'\n", filename);

    while (fgets(input, sizeof(input), file)) {
      // 去掉末尾换行符
      input[strcspn(input, "\n")] = '\0';

      int argc_parsed = parse_input(input, args);

      if (argc_parsed == 0) {
        continue;  // 空行
      }

      // 处理内置命令
      if (is_builtin_command(args)) {
        free_args(args);
        continue;
      }

      // 处理自定义命令
      const char *cmd_name = args[0];
      const char *cmd_arg1 = (argc_parsed >= 2) ? args[1] : NULL;
      const char *cmd_arg2 = (argc_parsed >= 3) ? args[2] : NULL;

      printf("cmd_name: %s\n", cmd_name);
      printf("cmd_arg1: %s\n", cmd_arg1);
      printf("cmd_arg2: %s\n", cmd_arg2);

      int found = 0;
      for (Command *cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd_name, cmd->name) == 0) {
          found = 1;
          if (cmd->is_arg_required == 0) {
            cmd->func.func_0();
          } else if (cmd->is_arg_required == 1) {
            cmd->func.func_1(cmd_arg1);
          } else if (cmd->is_arg_required == 2) {
            cmd->func.func_2(cmd_arg1, cmd_arg2);
          }
          break;
        }
      }

      if (!found) {
        fprintf(stderr, "mybash: command not found: %s\n", cmd_name);
      }

      free_args(args);
    }

    fclose(file);
    return 0;
  } 
  else {
    // 🔁 原有的交互式命令行模式
    while (1) {
      printf("mybash$ ");
      fflush(stdout);

      if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("\n");
        break;
      }

      input[strcspn(input, "\n")] = '\0';

      int argc = parse_input(input, args);

      if (argc == 0) {
        continue;
      }

      if (is_builtin_command(args)) {
        free_args(args);
        continue;
      }

      const char *cmd_name = args[0];
      const char *cmd_arg = (argc >= 2) ? args[1] : NULL;
      const char *cmd_arg2 = (argc >= 3) ? args[2] : NULL;

      int found = 0;
      for (Command *cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd_name, cmd->name) == 0) {
          found = 1;
          if (cmd->is_arg_required == 0) {
            cmd->func.func_0();
          } else if (cmd->is_arg_required == 1) {
            cmd->func.func_1(cmd_arg);
          } else if (cmd->is_arg_required == 2) {
            cmd->func.func_2(cmd_arg, cmd_arg2);
          }
          break;
        }
      }

      if (!found) {
        fprintf(stderr, "mybash: command not found: %s\n", cmd_name);
      }

      free_args(args);
    }
  }

  return 0;
}
