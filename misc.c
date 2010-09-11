#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include "nvram.h"

/*int print_env() {
	char *env = NULL, *nxt = NULL;

	if (!environment) {
		fprintf(stderr, "Error: environment not read\n");
		return -1;
	}

	for (env = environment; *env; env = nxt + 1) {
		for (nxt = env; *nxt; nxt++) {
			if (nxt >= &env[env_size]) {
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
}*/

/*int print_entries() {
	int i;

	printf("NAME               OFFSET          SIZE\n");
	printf("------------------------------------------\n");
	for (i=0; i<MAX_ENTRIES; i++) {
		if (!strncmp(entries[i].magic, ENTRY_MAGIC, MAGIC_LEN)) {
			printf("%-16s %#.8x     %#.8x\n", entries[i].name, entries[i].offset, entries[i].size);
		}
	}
}*/

/*int main(int argc, char **argv) {
	char *var = NULL, *val = NULL;
	if (argc < 2 || argc > 4) {
		printf("usage: fw_setenv <var> [<var>]\n");
		return -1;
	}

	var = argv[1];
	if (argc > 2)
		val = argv[2];

	read_entries();
	read_env();
	set_env(var, val);

	return 0;
}

int main2(int argc, char **argv) {
	printf("********************************************************\n");
	printf("                      NVRAM                             \n");
	printf("********************************************************\n");
	printf("\n");
	read_entries();
	print_entries();

	printf("\n\n");
	printf("********************************************************\n");
	printf("                    Bootie Env                          \n");
	printf("********************************************************\n");
	printf("\n");
	read_env();
	print_env();

	return 0;
}*/

void usage() {
	printf("usage: bootie-config [options]\n");
	printf("\t--get, -g   get env\n");
	printf("\t--set, -s   set env\n");
}

int main (int argc, char *argv[])
{
	opterr = 0;
	int option_index;
	int chr;
	char input_name[255];
	char output_name[255];
	input_name[0] = '\0';
	uint8_t in_data[0x40000];
	int fd;

	snprintf(input_name, 255, DEFAULT_TOKEN_INPUT);

	struct option opts[] = {
		{ "help", no_argument, 0, 'h' },
		{ "get", required_argument, 0, 'g' },
		{ "set", required_argument, 0, 's' },
	};

	while (1) {
		option_index = 0;
		chr = getopt_long(argc, argv, "g:s:h", opts, &option_index);

		if (chr == -1)
			break;

		switch (chr) {
		case 'g':
			/* get env */
			break;
		case 'o':
			/* set env */
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	return 0;
}
