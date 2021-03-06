#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
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

int mount_tokens(struct nvram_entry *entries) {
	uint32_t size;
	struct token_header *header;
	int retval;
	struct stat st;
	char *data;
	char *tokens = read_entry("tokens", &size, entries);
	char tokendata[4000];
	header = (struct token_header *)tokens;
	data = tokens + sizeof (struct token_header);

	if(stat(MOUNT_PATH, &st) != 0) {
		retval = mkdir(MOUNT_PATH, S_IRUSR|S_IRGRP|S_IROTH);
		if ( retval == -1 ) {
			printf("Failed to create directory: %s\n", MOUNT_PATH);
			printf("The error returned was: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		else
			printf("Created directory %s\n", MOUNT_PATH);
	}
	else
		printf("%s Already exists... not creating it\n", MOUNT_PATH);

	while (header && !strncmp(header->magic, TOKEN_MAGIC, MAGIC_LEN)) {
		sprintf(tokendata,"%.*s",header->length, data);
		write_token_file(tokendata,header->name);
		header = (struct token_header *)(((int)header + header->length + sizeof(struct token_header) + 3) & ~3);
		data = (char *)header + sizeof(struct token_header);
	}
	free(tokens);

	if(stat(DEV_PATH, &st) != 0) {
		retval = symlink(MOUNT_PATH, DEV_PATH);
		if ( retval == -1 ) {
			printf("Failed to create symlink: %s\n", DEV_PATH);
			printf("The error returned was: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		else
			printf("Created symlink %s to %s\n", DEV_PATH, MOUNT_PATH);
	}
	else
		printf("%s Already exists... not creating it\n", DEV_PATH);

	return 0;
}

int write_token_file(char tokendata[], char tokenname[]) {
	char tokenpath[40];
	int tokenlen = strlen(tokendata);
	int retval;

	sprintf(tokenpath, "%s/%s", MOUNT_PATH, tokenname);
	int tokenfile=open(tokenpath, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IRGRP|S_IROTH);
	if ( write(tokenfile, tokendata, tokenlen) == tokenlen ) {
		printf("Created token file %s/%s\n", MOUNT_PATH, tokenname);
	}
	else {
		printf("Error writing token file %s/%s\n", MOUNT_PATH, tokenname);
		printf("The error returned was: %s\n", strerror(errno));
	}
	close(tokenfile);
	return 0;
}

int print_backup_tokens(struct nvram_entry *entries) {
	uint32_t size;
	struct token_header *header;
	char *data;
	char *tokens = read_entry("token-backup", &size, entries);

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

int print_all(struct nvram_entry *entries) {
	printf("********************************************************\n");
	printf("                      NVRAM                             \n");
	printf("********************************************************\n");
	printf("\n");
	print_entries(entries);

	printf("\n\n");
	printf("********************************************************\n");
	printf("                    Bootie Env                          \n");
	printf("********************************************************\n");
	printf("\n");
	print_env(entries);

	printf("\n\n");
	printf("********************************************************\n");
	printf("                      Tokens                            \n");
	printf("********************************************************\n");
	printf("\n");
	print_tokens(entries);
}

void print_usage() {
	printf("usage: bootie-config [options]\n");
	printf("\t--print-all\t\t\tPrints NVRAM headers, tokens and bootie env\n");
	printf("\t--print-nvram\t\t\tPrints the NVRAM layout\n");
	printf("\t--print-env\t\t\tPrints Bootie's env\n");
	printf("\t--print-tokens\t\t\tPrints Bootie's tokens\n");
	printf("\t--print-backup\t\t\tPrints Bootie's backup tokens\n");
	printf("\n\t--set-env <key> [<value>]\tSet Bootie env var\n");
	printf("\t--clear-env\t\t\tClear Bootie env\n");
	printf("\t--set-token <key> [<value>]\tSet Bootie token\n");
	printf("\t--restore-tokens\t\tRestore tokens from backups\n");
	printf("\t--mount-tokens\t\t\tMount tokens to /dev/tokens\n");
}

int main (int argc, char *argv[]) {

	int option_index;
	int chr;
	int action = 0;
	char *key = 0;
	char *value = 0;

	struct option opts[] = {
			{ "print-all", no_argument, &action, PRINT_ALL },
			{ "print-nvram", no_argument, &action, PRINT_ENTRIES },
			{ "print-env", no_argument, &action, PRINT_ENV },
			{ "print-tokens", no_argument, &action, PRINT_TOKENS },
			{ "print-backup", no_argument, &action, PRINT_BACKUP_TOKENS },
			{ "set-env", required_argument, &action, SET_ENV },
			{ "clear-env", no_argument, &action, CLEAR_ENV },
			{ "set-token", required_argument, &action, SET_TOKENS },
			{ "restore-tokens", no_argument, &action, RESTORE_TOKENS },
			{ "mount-tokens", no_argument, &action, MOUNT_TOKENS },
			{ "help", no_argument, 0, 'h' },
			{0, 0, 0, 0}
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
		case 0:
			if (action == SET_ENV || action == SET_TOKENS) {
				key = optarg;
				if (optind < argc && argv[optind][0] != '-') {
					value = argv[optind];
				}
			}
		default:
			break;
		}
	}

	if (action) {
		struct nvram_entry entries[MAX_ENTRIES];
		read_entries(entries);

		switch (action) {
		case PRINT_ALL:
			print_all(entries);
			break;
		case PRINT_ENTRIES:
			print_entries(entries);
			break;
		case PRINT_ENV:
			print_env(entries);
			break;
		case PRINT_TOKENS:
			print_tokens(entries);
			break;
		case PRINT_BACKUP_TOKENS:
			print_backup_tokens(entries);
			break;
		case SET_ENV:
			set_env(key, value, entries);
			break;
		case SET_TOKENS:
			set_token(key, value, entries);
			break;
		case RESTORE_TOKENS:
			restore_tokens(entries);
			break;
		case MOUNT_TOKENS:
			mount_tokens(entries);
			break;
		case CLEAR_ENV:
			printf("clear-env not implemented yet\n");
			break;
		}
	}
	else {
		print_usage();
	}
	return 0;
}
