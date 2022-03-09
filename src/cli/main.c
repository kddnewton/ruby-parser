#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "parse.h"

static int parse_file(const char *command, const char *path) {
  // Open the file for reading
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return EXIT_FAILURE;
  }

  // Stat the file to get the file size
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    close(fd);
    perror("fstat");
    return EXIT_FAILURE;
  }

  // mmap the file descriptor to virtually get the contents
  off_t size = sb.st_size;
  const char *source = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

  close(fd);
  if (source == MAP_FAILED) {
    perror("mmap");
    return EXIT_FAILURE;
  }

  if (strncmp(command, "tokenize", 8) == 0) {
    tokenize(size, source);
  } else if (strncmp(command, "parse", 5) == 0) {
    parse(size, source, &printer);
  }

  // Clean up and free
  munmap((void *) source, size);
  return EXIT_SUCCESS;
}

static int parse_stdin(const char *command) {
  char source[1024];
  size_t size = 0;

  while (fgets(source + size, 1024, stdin) != NULL) {
    size += strlen(source + size);
  }

  if (strncmp(command, "tokenize", 8) == 0) {
    tokenize(size, source);
  } else if (strncmp(command, "parse", 5) == 0) {
    parse(size, source, &printer);
  }

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return argc == 3 ? parse_file(argv[1], argv[2]) : parse_stdin(argv[1]);
}
