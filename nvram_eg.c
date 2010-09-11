#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "nvram_eg.h"

#define NVRAM_MAGIC "NVRM"
#define ENTRY_MAGIC "TOC1"
#define TOKEN_MAGIC "TOKN"
#define DEVICE "/dev/mmcblk0p1"
#define MAX_ENTRIES 127

struct nvram_entry entries[MAX_ENTRIES];
unsigned char *environment = NULL;
char *tokens = NULL;

int find_entry(char *name, unsigned long *offset, unsigned long *size) {
  int i;
  
  for (i=0; i<MAX_ENTRIES; i++) {
    if (strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN)) {
      break;
    }
    if (!strncmp(entries[i].name, name, NAME_LEN)) {
      *offset = entries[i].offset;
      *size = entries[i].size;
      break;
    }
  }

  return 0;
}

int read_env() {
  unsigned char *env = NULL, *nxt = NULL;
  int fd, count;
  unsigned long offset = 0, size = 0;

  find_entry("env", &offset, &size);

  if (!offset || !size) {
    fprintf(stderr, "Could not find environment offset 0x%x, size 0x%x\n", offset, size);
    return -1;
  }

  fd = open(DEVICE, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s for reading\n", DEVICE);
    return -1;
  }

  environment = calloc(1, size);
  if (!environment) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    close(fd);
    return -1;
  }

  lseek(fd, offset, SEEK_SET);
  count = read(fd, environment, size);
  if (count != size) {
    fprintf(stderr, "Error: read returned %d\n", count);
    free(environment);
    close(fd);
    environment = NULL;
    return -1;
  }

  close(fd);
  return 0;
}

int print_env() {
  char *env = NULL, *nxt = NULL;
  unsigned long offset = 0, size = 0;

  if (!environment) {
    fprintf(stderr, "Error: environment not read\n");
    return -1;
  }

  find_entry("env", &offset, &size);
  for (env = environment; *env; env = nxt + 1) {
    for (nxt = env; *nxt; nxt++) {
      if (nxt >= &env[size]) {
        fprintf(stderr, "## Error: environment not terminated\n");
        return -1;
      }
    }
  }

  env = environment;
  while (*env) {
    printf("%s = %s\n", env, env + strlen(env) + 1);
    env += strlen(env) + 1;
    env += strlen(env) + 1;
  }

  return 0;
}

int read_entries() {
  int count = 0;
  int i;
  int fd;
  struct nvram_header header;
 
  fd = open(DEVICE, O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Could not open %s for reading\n", DEVICE);
    return -1;
  }

  read(fd, &header, sizeof(struct nvram_header));

  for (i=0; i<MAX_ENTRIES; i++) {
    read(fd, &entries[i], sizeof(struct nvram_entry));
    if (strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN))
      break;
  }

  close(fd);
  return i;
}

int print_entries() {
  int i;

  printf("NAME               OFFSET          SIZE\n");
  printf("------------------------------------------\n");
  for (i=0; i<MAX_ENTRIES; i++) {
    if (!strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN)) {
      printf("%-16s %#.8x     %#.8x\n", entries[i].name, entries[i].offset, entries[i].size);
    }
  }
}

int print_tokens() {
  struct token_header *header;
  char *data;
  
  header = (struct token_header *)tokens;
  data = tokens + sizeof (struct token_header);

  while (header && !strncmp(header->magic, TOKEN_MAGIC, MAGIC_LEN)) {
    printf("%s = %s\n", header->name, data);
    header = (struct token_header *)(((int)header + header->length + sizeof(struct token_header) + 3) & ~3);
    data = (char *)header + sizeof(struct token_header);
  }
}

int read_tokens() {
  int fd;
  int count = 0;
  unsigned long offset = 0, size = 0;
  struct token_header header;

  fd = open(DEVICE, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s for reading\n", DEVICE);
    return -1;
  }

  find_entry("tokens", &offset, &size);

  if (!offset || !size) {
    fprintf(stderr, "Could not find tokens offset 0x%x, size 0x%x\n", offset, size);
    return -1;
  }

  tokens = (char *)memalign(4, size);
  if (!tokens) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    close(fd);
    return -1;
  }

  lseek(fd, offset, SEEK_SET);
  count = read(fd, tokens, size);
  if (count != size) {
    fprintf(stderr, "Error: read returned %d\n", count);
    free(tokens);
    close(fd);
    tokens = NULL;
    return -1;
  }

  close(fd);
  return 0;
}

int main(int argc, char **argv) {
  printf("********************************************************\n");
  printf("                      NVRAM                             \n");
  printf("********************************************************\n");
  printf("\n");
  read_entries();
  print_entries();

  printf("\n\n");
  printf("********************************************************\n");
  printf("                    Bootie Env                          \n");
  printf("********************************************************\n");
  printf("\n");
  read_env();
  print_env();

  printf("\n\n");
  printf("********************************************************\n");
  printf("                      Tokens                            \n");
  printf("********************************************************\n");
  printf("\n");
  read_tokens();
  print_tokens();

  return 0;
}
