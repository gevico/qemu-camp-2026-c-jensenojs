#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

void print_elf_type(uint16_t e_type) {
  const char *type_str;
  switch (e_type) {
      case ET_REL:
        type_str = "Relocatable";
        break;
      case ET_EXEC:
        type_str = "Executable";
        break;
      case ET_DYN:
        type_str = "Shared Object/PIE";
        break;
      default:
        type_str = "Unknown";
        break;
  }
  printf("ELF Type: %s (0x%x)\n", type_str, e_type);
}

int main(int argc, char *argv[]) {
  char filepath[2][256] = {
    "./17_myfile.o",
    "./17_myfile",
  };

  int fd;
  Elf64_Ehdr ehdr;

  (void)argc;
  (void)argv;

  for (int i = 0; i < 2; i++) {
    fd = open(filepath[i], O_RDONLY);
    if (fd < 0) {
      perror(filepath[i]);
      continue;
    }

    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
      fprintf(stderr, "Failed to read ELF header: %s\n", filepath[i]);
      close(fd);
      continue;
    }

    close(fd);
    printf("%s: ", filepath[i]);
    print_elf_type(ehdr.e_type);
  }


  return 0;
}
