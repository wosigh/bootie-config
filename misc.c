#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include "nvram.h"

int print_entries(struct nvram_entry *entries) {
	int i;

	printf("NAME               OFFSET          SIZE\n");
	printf("------------------------------------------\n");
	for (i=0; i<MAX_ENTRIES; i++) {
		if (!strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN)) {
			printf("%-16s %#.8x     %#.8x\n", entries[i].name, entries[i].offset, entries[i].size);
		}
	}
}

int print_env(char *environment, uint32_t size) {
	char *env = NULL, *nxt = NULL;

	if (!environment) {
		fprintf(stderr, "Error: environment not read\n");
		return -1;
	}

	for (env = environment; *env; env = nxt + 1) {
		for (nxt = env; *nxt; nxt++) {
			if (nxt >= size) {
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

void print_usage() {
	printf("usage: bootie-config [options]\n");
	printf("\t--get, -g   get env\n");
	printf("\t--set, -s   set env\n");
}

int main (int argc, char *argv[]) {

	int option_index;
	int chr;

	struct option opts[] = {
			{ "help", no_argument, 0, 'h' },
	};

	while (1) {
		option_index = 0;
		chr = getopt_long(argc, argv, "h", opts, &option_index);

		if (chr == -1)
			break;

		switch (chr) {
		case 'h':
			print_usage();
			return 0;
		default:
			break;
		}
	}

	uint32_t size;
	struct nvram_entry entries[MAX_ENTRIES];
	read_entries(entries);

	print_entries(entries);

	char *environment = read_entry("env", &size, entries);
	print_env(environment, size);
	free(environment);

	char *tokens = read_entry("tokens", &size, entries);
	print_tokens(tokens);
	free(tokens);

	return 0;
}
