/*
 * test_dictionary.c
 *
 * 2013-12-16 Steven Wart created this file
 */

#include <stdio.h>
#include <string.h>	// strdup(), strcmp()
#include <stdlib.h>	// free()

#include "dictionary.h"

long
load_words(dictionary_t *dict, const char *filename)
{
	FILE *input = fopen(filename, "r");

	if (!input)
	{
		char error[256];
		sprintf(error, "Unable to open file %s", filename);
		perror(error);
		return 0;
	}

	char line[256];

	long bytes_allocated = 0;
	long count = 0;

	while (fgets(line, 256, input)) {
		size_t len = strlen(line);
		if (len == 0)
			continue;
		if (line[len-1] == '\n')
			line[--len] = '\0';

		char *new_string = strdup(line);
		char *replaced_value = (char *)dictionary_put(dict, line, (dict_value_t)new_string);

		// if we overwrote a key, we need to free the value that was replaced
		if (replaced_value) {
			bytes_allocated -= (strlen(replaced_value)+1);
			free(replaced_value);
		}

		char *value = (char *)dictionary_get(dict, line);
		if (strcmp(value, new_string) != 0) {
			printf("Error found in load_words(), found value '%s', was expecting '%s' for key '%s'\n",
				value, new_string, line);
		}
		count++;
		bytes_allocated += (len+1);
	}

	printf("\n %lu entries,  %lu collisions, maximum chain = %lu, %lu bytes allocated\n",
		dict->num_entries, dict->num_collisions, dict->maximum_chain, bytes_allocated);

	fclose(input);

	return bytes_allocated;
}

long
unload_words(dictionary_t *dict, const char *filename)
{
	FILE *input = fopen(filename, "r");

	if (!input)
	{
		char error[256];
		sprintf(error, "unload_words(): Unable to open file %s", filename);
		perror(error);
		return 0;
	}

	char line[256];
	long bytes_freed = 0;
	long count = 0;

	FILE *unloaded_words = fopen("unloaded_words.txt", "w");

	while (fgets(line, 256, input)) {
		size_t len = strlen(line);
		if (len == 0)
			continue;
		if (line[len-1] == '\n')
			line[--len] = '\0';

		count++;
		if ((count % 10000) == 0) {
			printf("	removed %lu items\n", count);
		}
		char *removed_value = (char *)dictionary_remove(dict, line);

		if (removed_value) {
			bytes_freed += (strlen(removed_value)+1);
			fprintf(unloaded_words, "%s\n", (char *)removed_value);
			free(removed_value);
		}
	}

	fclose(unloaded_words);
	fclose(input);

	return bytes_freed;
}

// globals because no nested functions in ANSI C
long count = 0;
long num_entries = 0;
long bytes_freed = 0;
FILE *freed_words = NULL;

void
free_value(dict_key_t key, dict_value_t value)
{
	bytes_freed += (strlen(value) + 1);
	fprintf(freed_words, "%s\n", (char *)value);
	free((char *)value);
	count++;
}

long
free_words(dictionary_t *dict)
{
	count = 0;
	bytes_freed = 0;

	freed_words = fopen("freed_words.txt", "w");

	printf("Freeing values...");
	dictionary_enumerate(dict, &free_value);
	printf("%lu strings freed, %lu bytes\n", count, bytes_freed);

	fclose(freed_words);
	return bytes_freed;
}

void
print_entry(dict_key_t key, dict_value_t value)
{
	if (count++ == 30) {
		printf("skipping %lu entries...\n", num_entries-40);
	}
	else if ((count < 30) || (count > num_entries-10)) {
		printf("\t%s:%s (%p)\n", key, (char *)value, (void *)value);
	}
}

void
test_dictionary_enum(dictionary_t *dict)
{
	count = 0;
	num_entries = dict->num_entries;	// no nested functions in ANSI C :(

	printf("Enumerating dictionary:\n");
	dictionary_enumerate(dict, &print_entry);
	printf("\n");
}

/*
 * Read lines one at a time from the file, add keys and values to
 * a new dictionary, print the contents of the dicionary, and free it
 */
void
test_load(char *filename, long size, double load_factor)
{
	dictionary_t *dict = new_dictionary_size_load(size, load_factor);

	printf("Loading dictionary entries from file %s\n", filename);

	// load the dictionary with values
	long bytes_allocated = load_words(dict, filename);

	test_dictionary_enum(dict);

	// free all the values we added
	long bytes_freed = free_words(dict);

	if (bytes_allocated != bytes_freed) {
		printf("Failed to free %lu bytes: %lu allocated, only %lu were freed\n",
			bytes_allocated - bytes_freed, bytes_allocated, bytes_freed);
	}

	free_dictionary(dict);
}

/*
 * Read lines one at a time from the file, add keys and values to
 * a new dictionary, print the contents of the dicionary, make a second pass through the file
 * to remove all the elements one at a time, then free it.
 */
void
test_unload(char *filename, long size, double load_factor)
{
	dictionary_t *dict = new_dictionary_size_load(size, load_factor);

	printf("Testing dictionary_remove()...\n");

	printf("1) Rebuilding dictionary from %s\n", filename);

	// load the dictionary with values = key
	long bytes_allocated = load_words(dict, filename);

	printf("2) Re-reading dictionary entries to unload from %s\n", filename);

	long bytes_freed = unload_words(dict, filename);

	if (bytes_allocated != bytes_freed) {
		printf("Failed to free %lu bytes: %lu allocated, only %ld were freed\n",
			bytes_allocated - bytes_freed, bytes_allocated, bytes_freed);
	}
	else {
		printf("3) All entries have been removed.\n");
	}

	test_dictionary_enum(dict);

	free_dictionary(dict);
}

int
main(int argc, char **argv)
{
	if (argc == 1)
		goto usage;

	long size = 5;
	double load_factor = LOAD_FACTOR;
	char * filename = NULL;

	for (int i=1; i < argc; i++) {
		if (strcmp("--size", argv[i]) == 0) {
			size = atol(argv[i+1]);
			i++;
		}
		else if (strcmp("--load", argv[i]) == 0) {
			load_factor = atof(argv[i+1]);
			i++;
		}
		else {
			filename = argv[i];
		}
	}

	if (filename == NULL)
		goto usage;

	test_load(filename, size, load_factor);

	// repeat the test, but instead of deallocating, use the remove function
	test_unload(filename, size, load_factor);

	return 0;

usage:
	printf("usage: test_dictionary <filename> [--size <size>] [--load <load_factor>]\n");
	printf("	This program will read lines one at a time from a file\n");
	printf("	It will add each line as the keys and values of a new dictionary,\n");
	printf("	print some statistics, the contents of the dictionary, and free it.\n");

	return 1;
}
