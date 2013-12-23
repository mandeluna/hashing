/*
 * djb_hash.c - hash the string provided on stdin
 *
 * If a second argument is provided, print the result modulo that value
 *
 * 2013-12-17 Steven Wart created this file
 */

#include <stdio.h>
#include "hash.h"

int
main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: %s <string> [modulus]\n", argv[0]);
		return 0;
	}

	long modulus = -1;

	if (argc >= 3) {
		modulus = atol(argv[2]);
		if (modulus <= 0) {
			printf("Error: invalid modulus %lu\n", modulus);
			return 0;
		}
		printf("%lu\n", hash(argv[1]) % modulus);
	}
	else {
		printf("%lu\n", hash(argv[1]));
	}

	return 1;
}