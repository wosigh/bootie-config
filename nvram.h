#ifndef NVRAM_H_
#define NVRAM_H_

#include <stdint.h>

#define NVRAM_MAGIC "NVRM"
#define ENTRY_MAGIC "TOC1"
#define TOKEN_MAGIC "TOKN"
#define DEVICE "/dev/mmcblk0p1"
#define MAX_ENTRIES 127
#define NAME_LEN 16
#define MAGIC_LEN 4

enum {
  OPT_NONE,
  PRINT_ALL,
  PRINT_ENTRIES,
  PRINT_ENV,
  PRINT_TOKENS,
  PRINT_BACKUP_TOKENS,
  SET_ENV,
  CLEAR_ENV,
  SET_TOKENS,
  RESTORE_TOKENS
};

struct nvram_header {
  char magic[MAGIC_LEN]; // NVRM
  uint32_t version;
  uint32_t size;
  uint32_t crc;
  uint32_t padding[4];
};

struct nvram_entry {
  char magic[MAGIC_LEN]; // TOC1
  uint32_t offset;
  uint32_t size;
  uint32_t version;
  char name[NAME_LEN];
};

struct token_header {
  char magic[MAGIC_LEN]; // TOKN
  uint32_t version;
  uint32_t length;
  uint32_t gen;
  uint32_t crc;
  char name[NAME_LEN];
};

struct token {
	struct token_header header;
	char *data;
};

int read_entries(struct nvram_entry *entries);
int find_entry(char *name, struct nvram_entry *entries);
char * read_entry(char *entry, uint32_t *size, struct nvram_entry *entries);
int set_env(char *newvar, char *newval, struct nvram_entry *entries);
int set_token(char *, char *, struct nvram_entry *);

#endif /* NVRAM_H_ */
