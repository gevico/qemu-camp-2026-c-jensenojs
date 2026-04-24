#include "mywc.h"

// 创建哈希表
WordCount **create_hash_table() {
  WordCount **hash_table = calloc(HASH_SIZE, sizeof(WordCount *));
  return hash_table;
}

// 简单的哈希函数
unsigned int hash(const char *word) {
  unsigned long hash = 5381;
  int c;
  while ((c = *word++))
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  return hash % HASH_SIZE;
}

// 检查字符是否构成单词的一部分
bool is_valid_word_char(char c) { return isalpha(c) || c == '\''; }

// 转换为小写
char to_lower(char c) { return tolower(c); }

// 添加单词到哈希表
void add_word(WordCount **hash_table, const char *word) {
    unsigned int id = hash(word);
    WordCount *cur = hash_table[id];
    while(cur) {
        if (strcmp(cur->word, word)==0) {
            cur->count++;
            return;
        }
        cur = cur->next;
    }
    // head insert
    WordCount *new = (WordCount*)malloc(sizeof(WordCount));
    strcpy(new->word, word);
    new->count = 1;
    new->next = hash_table[id];
    hash_table[id] = new;
}

// 打印单词统计结果
void print_word_counts(WordCount **hash_table) {
  printf("Word Count Statistics:\n");
  printf("======================\n");

  for (int i = 0; i < HASH_SIZE; i++) {
    WordCount *cur = hash_table[i];
    while (cur) {
      printf("%-20s %d\n", cur->word, cur->count);
      cur = cur->next;
    }
  }
}

// 释放哈希表内存
void free_hash_table(WordCount **hash_table) {
    for (int i = 0; i < HASH_SIZE; i++) {
      WordCount *cur = hash_table[i];
      while (cur) {
        WordCount *next = cur->next;
        free(cur);
        cur = next;
      }
    }
    free(hash_table);
}

// 处理文件并统计单词
void process_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  WordCount **hash_table = create_hash_table();
  char word[MAX_WORD_LEN];
  int word_pos = 0;
  int c;

  while ((c = fgetc(file)) != EOF) {
    if (is_valid_word_char(c)) {
      if (word_pos < MAX_WORD_LEN - 1) {
        word[word_pos++] = to_lower(c);
      }
    } else {
      if (word_pos > 0) {
        word[word_pos] = '\0';
        add_word(hash_table, word);
        word_pos = 0;
      }
    }
  }

  // 处理文件末尾的最后一个单词
  if (word_pos > 0) {
    word[word_pos] = '\0';
    add_word(hash_table, word);
  }

  fclose(file);
  print_word_counts(hash_table);
  free_hash_table(hash_table);
}
