#include <stdint.h>

#define NAME_LEN 16
#define MAGIC_LEN 4

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
