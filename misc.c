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

	return 0;
}

int print_env(struct nvram_entry *entries) {
	uint32_t size;
	char *env = NULL, *nxt = NULL;
	char *environment = read_entry("env", &size, entries);

	env = environment;
	while (*env) {
		printf("%s = %s\n", env, env + strlen(env) + 1);
		env += strlen(env) + 1;
		env += strlen(env) + 1;
	}
	free(environment);

	return 0;
}

int print_tokens(struct nvram_entry *entries) {
	uint32_t size;
	struct token_header *header;
	char *data;
	char *tokens = read_entry("tokens", &size, entries);

	header = (struct token_header *)tokens;
	data = tokens + sizeof (struct token_header);

	while (header && !strncmp(header->magic, TOKEN_MAGIC, MAGIC_LEN)) {
		printf("%s = %.*s\n", header->name, header->length, data);
		header = (struct token_header *)(((int)header + header->length + sizeof(struct token_header) + 3) & ~3);
		data = (char *)header + sizeof(struct token_header);
	}
	free(tokens);

	return 0;
}

void print_usage() {
	printf("usage: bootie-config [options]\n");
	printf("\t--print-nvram\t\tPrints the NVRAM layout\n");
	printf("\t--print-env\t\tPrints Bootie's env\n");
	printf("\t--print-tokens\t\tPrints Bootie's tokens\n");
	printf("\n\t--set-env\t\tSet Bootie env var\n");
	//printf("\t--set-token\t\tSet Bootie token\n");
	printf("\n\t-k, --key\t\tkey\n");
	printf("\t-v, --value\t\tvalue\n");
}

int main (int argc, char *argv[]) {

	int option_index;
	int chr;
	int action = 0;
	char *key = 0;
	char *value = 0;

	struct option opts[] = {
			{ "print-nvram", no_argument, &action, 1 },
			{ "print-env", no_argument, &action, 2 },
			{ "print-tokens", no_argument, &action, 3 },
			{ "set-env", no_argument, &action, 4 },
			{ "set-token", no_argument, &action, 5 },
			{ "help", no_argument, 0, 'h' },
			{ "key", required_argument, 0, 'k'},
			{ "value", required_argument, 0, 'v'},
			{0, 0, 0, 0}
	};

	while (1) {
		option_index = 0;
		chr = getopt_long(argc, argv, "ht:k:v:", opts, &option_index);

		if (chr == -1)
			break;

		switch (chr) {
		case 'h':
			print_usage();
			return 0;
		case 'k':
			key = optarg;
			break;
		case 'v':
			value = optarg;
			break;
		default:
			break;
		}
	}

	if (action) {
		struct nvram_entry entries[MAX_ENTRIES];
		read_entries(entries);

		switch (action) {
		case 1:
			print_entries(entries);
			break;
		case 2:
			print_env(entries);
			break;
		case 3:
			print_tokens(entries);
			break;
		case 4:
			set_env(key, value, entries);
			break;
		/*case 5:
			set_tokens(key, value, entries);
			break;*/
		}
	}
  else {
    print_usage();
  }
	return 0;
}
