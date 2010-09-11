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

	char *environment = 0;
	int fd, count;

	int e = find_entry(entry, entries);

	if (!entries[e].offset || !entries[e].size) {
		fprintf(stderr, "Could not find '%s' offset 0x%x, size 0x%x\n", entry, entries[e].offset, entries[e].size);
		return environment;
	}

	fd = open(DEVICE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for reading\n", DEVICE);
		return environment;
	}

	environment = calloc(1, entries[e].size);
	if (!environment) {
		fprintf(stderr, "Error: Memory allocation failed\n");
		close(fd);
		return environment;
	}

	lseek(fd, entries[e].offset, SEEK_SET);
	count = read(fd, environment, entries[e].size);
	if (count != entries[e].size) {
		fprintf(stderr, "Error: read returned %d\n", count);
		free(environment);
		close(fd);
		return environment;
	}

	close(fd);

	*size = entries[e].size;
	return environment;
}

int read_tokens(char *tokens, struct nvram_entry *entries) {
	int fd;
	int count = 0;
	struct token_header header;

	fd = open(DEVICE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for reading\n", DEVICE);
		return -1;
	}

	int e = find_entry("tokens", entries);

	if (!entries[e].offset || !entries[e].size) {
		fprintf(stderr, "Could not find tokens offset 0x%x, size 0x%x\n", entries[e].offset, entries[e].size);
		return -1;
	}

	memalign(4, entries[e].size);
	if (!tokens) {
		fprintf(stderr, "Error: Memory allocation failed\n");
		close(fd);
		return -1;
	}

	lseek(fd, entries[e].offset, SEEK_SET);
	count = read(fd, tokens, entries[e].size);
	if (count != entries[e].size) {
		fprintf(stderr, "Error: read returned %d\n", count);
		free(tokens);
		close(fd);
		tokens = NULL;
		return -1;
	}

	close(fd);
	return 0;
}
