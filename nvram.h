#ifndef NVRAM_H_
#define NVRAM_H_

#include <stdint.h>

#define NVRAM_MAGIC "NVRM"
#define ENTRY_MAGIC "TOC1"
#define DEVICE "/dev/mmcblk0p1"
#define MAX_ENTRIES 127

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
	uint32_t size;
	uint32_t gen;
	uint32_t crc;
	char name[16];
};

struct token {
	struct token_header header;
	char *data;
};

void find_env();
int read_env();
int read_entries();
int write_env();
int set_env(char *newvar, char *newval);

//struct nvram_entry entries[MAX_ENTRIES];
//char *environment = NULL;
//unsigned long env_offset = 0;
//unsigned long env_size = 0;

#endif /* NVRAM_H_ */
