
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "nvram.h"

#define NVRAM_MAGIC "NVRM"
#define ENTRY_MAGIC "TOC1"
#define TOKEN_MAGIC "TOKN"
#define DEVICE "/dev/mmcblk0p1"
#define MAX_ENTRIES 127

struct nvram_entry entries[MAX_ENTRIES];
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
    fprintf(stderr, "Error: (read) Could not find tokens offset 0x%x, size 0x%x\n", offset, size);
    return -1;
  }

  tokens = (char *)memalign(4, size);
  memset(tokens, 0, size);
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

int write_tokens(char *tokens) {
  int fd, count;
  unsigned long offset = 0, size = 0;

  if (!tokens) {
    fprintf(stderr, "Tokens not read\n");
    return -1;
  }

  find_entry("tokens", &offset, &size);

  if (!offset || !size) {
    fprintf(stderr, "Could not find tokens size 0x%x, size 0x%x\n", offset, size);
    return -1;
  }

  fd = open(DEVICE, O_WRONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s for writing\n", DEVICE);
    return -1;
  }

  printf("Writing tokens to nvram...\n");
  lseek(fd, offset, SEEK_SET);
  count = write(fd, tokens, size);
  if (count != size) {
    fprintf(stderr, "Error: write returned %d\n", count);
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int set_token(char *var, char *val) {
  char *new_tokens = NULL;
  char *ptr = NULL;
  unsigned long offset = 0, size = 0, length = 0;
  int fd;
  int found = 0;

  if (!var) {
    fprintf(stderr, "need var\n");
    return -1;
  }

  if (strlen(var) > NAME_LEN) {
    fprintf(stderr, "var name to big\n");
    return -1;
  }

  find_entry("tokens", &offset, &size);

  if (!offset || !size) {
    fprintf(stderr, "Error: (set) Could not find tokens offset 0x%x, size 0x%x\n", offset, size);
    return -1;
  }

  new_tokens = (char *)memalign(4, size);
  memset(new_tokens, 0, size);
  if (!new_tokens) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    close(fd);
    return -1;
  }

  struct token_header *header;
  struct token_header *new_header;
  char *data;
  
  ptr = new_tokens;
  header = (struct token_header *)tokens;
  data = tokens + sizeof (struct token_header);

  while (header) {
    if (!header->magic[0] || strncmp(header->magic, TOKEN_MAGIC, MAGIC_LEN)) {
      break;
    }
    length = sizeof(struct token_header);
    if (!strncmp(var, header->name, NAME_LEN)) {
      found = 1;
      if (val) {
        new_header = (struct token_header*)ptr;
        memcpy(new_header, header, sizeof (struct token_header));
        new_header->length = strlen(val);
        new_header->crc = 0;
        if ((int)new_header + sizeof(struct token_header) + new_header->length >=
            (int)new_tokens + size) {
          fprintf(stderr, "ERROR: not enough space to change\n");
          free(new_tokens);
          return -1;
        }
        length += ((new_header->length + 3) & ~3);
        memcpy(new_header+1, val, new_header->length);
        new_header->crc = crc32(0, new_header, length);
      }
      else {
        length = 0;
      }
    }
    else {
      length += ((header->length + 3) & ~3);
      memcpy(ptr, header, length);
    }

    ptr += length;
    header = (struct token_header *)(((int)header + header->length + sizeof(struct token_header) + 3) & ~3);
    data = (char *)header + sizeof(struct token_header);
  }

  if (!found) {
    if ((ptr + sizeof(struct token_header) + strlen(val)) > (new_tokens + size)) {
        fprintf(stderr, "ERROR: not enough space to add\n");
        free(new_tokens);
        return -1;
    }

    new_header = (struct token_header*)ptr;
    strcpy(new_header->magic, "TOKN");
    new_header->version = 1;
    new_header->length = strlen(val);
    new_header->gen = 0;
    new_header->crc = 0;
    strcpy(new_header->name, var);
    memcpy(new_header+1, val, (new_header->length + 3) & ~3);
    length = sizeof(struct token_header) + new_header->length;
    new_header->crc = crc32(0, new_header, length);
  }

#ifdef DEBUG
  print_tokens(new_tokens);
#else
  write_tokens(new_tokens);
#endif
  free(new_tokens);
  return 0;
}

int print_tokens(char *tokens) {
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

int main(int argc, char **argv) {
  char *var = NULL, *val = NULL;

  if (argc < 2 || argc > 4) {
    printf("usage: set_token <var> [<var>]\n");
    return -1;
  }

  var = argv[1];
  if (argc > 2)
    val = argv[2];

  read_entries();
  read_tokens();
  set_token(var, val);

  return 0;
}
