#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "nvram.h"

#define NVRAM_MAGIC "NVRM"
#define ENTRY_MAGIC "TOC1"
#define DEVICE "/dev/mmcblk0p1"
#define MAX_ENTRIES 127

struct nvram_entry entries[MAX_ENTRIES];
unsigned char *environment = NULL;
unsigned long env_offset = 0;
unsigned long env_size = 0;

void find_env() {
  int i;
  unsigned long offset = 0;
  
  for (i=0; i<MAX_ENTRIES; i++) {
    if (strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN)) {
      break;
    }
    if (!strncmp(entries[i].name, "env", NAME_LEN)) {
      env_offset = entries[i].offset;
      env_size = entries[i].size;
      break;
    }
  }
}

int read_env() {
  unsigned char *env = NULL, *nxt = NULL;
  int fd, count;

  find_env();

  if (!env_offset || !env_size) {
    fprintf(stderr, "Could not find environment offset 0x%x, size 0x%x\n", env_offset, env_size);
    return -1;
  }

  fd = open(DEVICE, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s for reading\n", DEVICE);
    return -1;
  }

  environment = calloc(1, env_size);
  if (!environment) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    close(fd);
    return -1;
  }

  lseek(fd, env_offset, SEEK_SET);
  count = read(fd, environment, env_size);
  if (count != env_size) {
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

  if (!environment) {
    fprintf(stderr, "Error: environment not read\n");
    return -1;
  }

  for (env = environment; *env; env = nxt + 1) {
    for (nxt = env; *nxt; nxt++) {
      if (nxt >= &env[env_size]) {
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

  return 0;
}