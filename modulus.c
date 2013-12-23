/*
 * modulus.c - compute the value of the first number modulus the second
 *
 * usage: modulus dividend op divisor
 *
 * 2013-12-18 Steven Wart created this file
 */

#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
{
	if (argc != 4) {
		goto usage;
	}

	long dividend = atol(argv[1]);
	char *op = argv[2];
	long divisor = atol(argv[3]);

	if (strcmp(op, "%") == 0) {
		printf("%lu (%lx) %% %lu (%lx) = %lu\n", dividend, dividend, divisor, divisor, dividend % divisor);
	}
	else if (strcmp(op, "&") == 0) {
		printf("%lu (%lx) & %lu (%lx) = %lu\n", dividend, dividend, divisor, divisor, dividend & divisor);
	}
	else
		goto usage;

	return 1;

usage:
	printf("usage: modulus <dividend> <op> <divisor>\n");
	printf("	dividend, divisor are long integers\n");
	printf("	op is either %% (modulus) or & (bitwise and)\n");
	return 0;
}