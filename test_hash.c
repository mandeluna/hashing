#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "hash.h"

#define NUM_ENTRIES	1000

const char *input_strings = "unsortedWords.txt";

void
test_hash()
{
	FILE *input = fopen(input_strings, "r");

	if (!input)
	{
		char error[256];
		sprintf(error, "Unable to open file %s", input_strings);
		perror(error);
		return;
	}

	int hash_table[NUM_ENTRIES];
	memset(hash_table, 0, NUM_ENTRIES * sizeof(int));
	char line[256];

	long count = 0;
	while (fgets(line, 256, input)) {
		size_t len = strnlen(line, 256);
		if (len == 0)
			continue;
		if (line[len-1] == '\n')
			line[--len] = '\0';
		long hash_value = hash(line);
		hash_table[hash_value % NUM_ENTRIES]++;

		count++;
	}

	fclose(input);

	long sum = 0;
	long sum_sq = 0;
	double mean = 0.0, variance = 0.0;
	int min = INT_MAX, max = 0;

	printf("Bucket\tCount\n");
	for (int i=0; i < NUM_ENTRIES; i++) {
		if (min > hash_table[i]) {
			min = hash_table[i];
		}
		if (max < hash_table[i]) {
			max = hash_table[i];
		}
		sum += hash_table[i];
		sum_sq += (hash_table[i] * hash_table[i]);
		printf("%4i\t%d\n", i, hash_table[i]);
	}
	mean = sum / NUM_ENTRIES;
	variance = sqrt(sum_sq / NUM_ENTRIES - mean * mean);

	printf("\n");
	printf("%lu entries distributed into %d buckets: sum=%lu, mean=%5.1f, variance=%5.1f, min=%d, max=%d\n",
		count, NUM_ENTRIES, sum, mean, variance, min, max);
}

int
main(int argc, char **argv)
{
	test_hash();
	return 0;
}
