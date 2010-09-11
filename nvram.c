#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "nvram.h"

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
		fprintf(stderr, "Could not find environment offset 0x%lu, size 0x%lu\n", env_offset, env_size);
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

int write_env() {
	int fd, count;

	if (!environment) {
		fprintf(stderr, "Environment not read\n");
		return -1;
	}

	find_env();

	if (!env_offset || !env_size) {
		fprintf(stderr, "Could not find environment offset 0x%lu, size 0x%lu\n", env_offset, env_size);
		return -1;
	}

	fd = open(DEVICE, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for writing\n", DEVICE);
		return -1;
	}

	printf("Writing environment to nvram...\n");
	lseek(fd, env_offset, SEEK_SET);
	count = write(fd, environment, env_size);
	if (count != env_size) {
		fprintf(stderr, "Error: write returned %d\n", count);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int set_env(char *newvar, char *newval) {
	int count = 0;
	char *env = NULL, *nxt = NULL;

	if (!newvar) {
		return -1;
	}

	if (!environment) {
		fprintf(stderr, "Environment not read\n");
		return -1;
	}

	for (env = environment; *env; env = nxt, count++) {
		for (nxt = env; *nxt; nxt++) {
			if (nxt >= &environment[env_size]) {
				fprintf(stderr, "Error: environment not terminated\n");
				return -1;
			}
		};
		for (nxt++; *nxt; nxt++) {
			if (nxt >= &environment[env_size]) {
				fprintf(stderr, "Error: environment not terminated\n");
				return -1;
			}
		};
		nxt++;

		if (!strcmp(env, newvar)) {
			memcpy(env, nxt, &env[env_size] - nxt);
		}
	}

	if (!newval)
		goto WRITE_FLASH;

	if ((nxt + strlen(newvar) + strlen(newval) + 3) >= &environment[env_size]) {
		fprintf(stderr, "Error: newvar will not fit.\n");
		return -1;
	}

	strcpy(nxt, newvar);
	nxt += strlen(nxt) + 1;
	strcpy(nxt, newval);
	WRITE_FLASH:
	return write_env();
}
