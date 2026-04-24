#include "myfile.h"
#include <stdio.h>
#include <string.h>

void print_elf_type(uint16_t e_type) {
    const char *type_str;
    switch (e_type) {
    case ET_NONE:
      type_str = "Unknown (ET_NONE)";
      break;
    case ET_REL:
      type_str = "Relocatable (ET_REL)";
      break;
    case ET_EXEC:
      type_str = "Executable (ET_EXEC)";
      break;
    case ET_DYN:
      type_str = "Shared Object/PIE (ET_DYN)";
      break;
    case ET_CORE:
      type_str = "Core Dump (ET_CORE)";
      break;
    default:
      if (e_type >= ET_LOOS && e_type <= ET_HIOS)
        type_str = "OS-Specific";
      else if (e_type >= ET_LOPROC && e_type <= ET_HIPROC)
        type_str = "Processor-Specific";
      else
        type_str = "Invalid";
    }
    printf("ELF Type: %s (0x%x)\n", type_str, e_type);
}

int __cmd_myfile(const char* filename) {
    char filepath[256];
    int fd;
    Elf64_Ehdr ehdr;

    strncpy(filepath, filename, sizeof(filepath) - 1);
    filepath[sizeof(filepath) - 1] = '\0';
    fflush(stdout);
    printf("filepath: %s\n", filepath);

    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
      perror("open");
      return 1;
    }

    lseek(fd, 0, SEEK_SET);
    ssize_t n = read(fd, &ehdr, sizeof(ehdr));
    if (n != sizeof(ehdr)) {
      perror("read");
      close(fd);
      return 1;
    }

    print_elf_type(ehdr.e_type);
    close(fd);
    return 0;
}
