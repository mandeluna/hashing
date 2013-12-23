/*
 * hash-input.c - read a bunch of lines from stdin, hash them, and write the combined values to stdout
 *
 * usage: hash-input [-m modulus] 
 *
 * 2013-12-18 Steven Wart created this file
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "hash.h"

#define NUM_ENTRIES	1000

int
hash_input(long modulus)
{
	char line[256];

	int count = 0;
	while (fgets(line, 256, stdin)) {
		size_t len = strlen(line);
		if (len == 0)
			continue;
		if (line[len-1] == '\n')
			line[--len] = '\0';
		long hash_value = hash(line) % ((modulus > 0) ? modulus : LONG_MAX);
		fprintf(stdout, "%lu\t%s\n", hash_value, line);
		count++;
	}
	return count;
}

int
main(int argc, char **argv)
{
	long modulus = 0;
	int count = 0;
	
	switch (argc) {
		case 3:
			if (strcmp(argv[1], "-m") == 0) {
				modulus = atol(argv[2]);
			}
		case 1:
			count = hash_input(modulus);
			fprintf(stderr, "%d lines processed from input\n", count);
			return 0;
			// else print usage
		default:
			printf("usage: hash-input [-m modulus]\n");
			return -1;
	}
}
