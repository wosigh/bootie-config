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

	if (!newvar || !newval) {
		return -1;
	}

	char *environment = read_entry("env", &size, entries);

	for (env = environment; *env; env = nxt, count++) {
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

	write_env(env, &entries[find_entry("env", entries)]);
	free(environment);
	free(env);

	return 0;
}
