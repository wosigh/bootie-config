#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "nvram.h"

int read_entries(struct nvram_entry *entries) {
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

int find_entry(char *name, struct nvram_entry *entries) {
	int i;

	for (i=0; i<MAX_ENTRIES; i++) {
		if (strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN)) {
			return i;
		}
		if (!strncmp(entries[i].name, name, NAME_LEN)) {
			return i;
		}
	}

	return -1;
}

char * read_entry(char *entry, uint32_t *size, struct nvram_entry *entries) {

	char *nvram = 0;
	int fd, count;

	int e = find_entry(entry, entries);

	if (!entries[e].offset || !entries[e].size) {
		fprintf(stderr, "Could not find '%s' offset 0x%x, size 0x%x\n", entry, entries[e].offset, entries[e].size);
		return nvram;
	}

	fd = open(DEVICE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for reading\n", DEVICE);
		return nvram;
	}

	nvram = memalign(4, entries[e].size);
  memset(nvram, 0, entries[e].size);
	if (!nvram) {
		fprintf(stderr, "Error: Memory allocation failed\n");
		close(fd);
		return nvram;
	}

	lseek(fd, entries[e].offset, SEEK_SET);
	count = read(fd, nvram, entries[e].size);
	if (count != entries[e].size) {
		fprintf(stderr, "Error: read returned %d\n", count);
		free(nvram);
    nvram = 0;
		close(fd);
		return nvram;
	}

	close(fd);

	*size = entries[e].size;
	return nvram;
}

int write_env(char *environment, struct nvram_entry *entry) {
	int fd, count;

	fd = open(DEVICE, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for writing\n", DEVICE);
		return -1;
	}

	printf("Writing environment to nvram...\n");
	lseek(fd, entry->offset, SEEK_SET);
	count = write(fd, environment, entry->size);
	if (count != entry->size) {
		fprintf(stderr, "Error: write returned %d\n", count);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int set_env(char *newvar, char *newval, struct nvram_entry *entries) {
	uint32_t size;
	int count = 0;
	char *env = NULL, *nxt = NULL;

	if (!newvar) {
		return -1;
	}

	char *environment = read_entry("env", &size, entries);

  if (!environment) {
    fprintf(stderr, "Environment not read\n");
    return -1;
  }

	for (nxt = env = environment; *env; env = nxt, count++) {
    for (nxt = env; *nxt; nxt++) {
      if (nxt >= &environment[size]) {
        fprintf(stderr, "Error: environment not terminated\n");
        return -1;
      }
    };
    for (nxt++; *nxt; nxt++) {
      if (nxt >= &environment[size]) {
        fprintf(stderr, "Error: environment not terminated\n");
        return -1;
      }
    };
    nxt++;

		if (!strcmp(env, newvar)) {
			memcpy(env, nxt, &env[size] - nxt);
		}
	}

	if (!newval)
		goto WRITE_FLASH;

	if ((nxt + strlen(newvar) + strlen(newval) + 3) >= &environment[size]) {
		fprintf(stderr, "Error: newvar will not fit.\n");
		return -1;
	}

	strcpy(nxt, newvar);
	nxt += strlen(nxt) + 1;
	strcpy(nxt, newval);

	WRITE_FLASH:

	write_env(environment, &entries[find_entry("env", entries)]);
	free(environment);

	return 0;
}

int dbg_print_tokens(char *tokens) {
  struct token_header *header;
  char *data;
  
  header = (struct token_header *)tokens;
  data = tokens + sizeof (struct token_header);

  while (header && !strncmp(header->magic, TOKEN_MAGIC, MAGIC_LEN)) {
    printf("%s = %.*s\n", header->name, header->length, data);
    header = (struct token_header *)(((int)header + header->length + sizeof(struct token_header) + 3) & ~3);
    data = (char *)header + sizeof(struct token_header);
  }
}

int set_token(char *var, char *val, struct nvram_entry *entries) {
  char *new_tokens = NULL;
  char *ptr = NULL;
  unsigned long size = 0, length = 0;
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

	char *tokens = read_entry("tokens", &size, entries);

  if (!tokens) {
    fprintf(stderr, "Environment not read\n");
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

#define DEBUG
#ifdef DEBUG
  dbg_print_tokens(new_tokens);
#else
  write_tokens(new_tokens);
#endif
  free(new_tokens);
  free(tokens);
  return 0;
}
