/*
 * dictionary.h
 *
 * 2013-12-16 Steven Wart created this file
 *
 */

#ifndef DICTIONARY

#define DICTIONARY

// this should be a prime number
// initially the dictionary is very small, but it will resize once the number of entries
// exceeds load factor * the current capacity of the dictionary
#define DICT_INITIAL_SIZE 	5
#define LOAD_FACTOR			0.75		// percentage

typedef char * dict_key_t;

typedef void * dict_value_t;

// Define a function returning void that takes key, value as arguments
// to be passed as the second argument to dictionary_enumerate()
typedef void (* dictionary_enumerator_t) (dict_key_t, dict_value_t);

typedef struct collision_bucket_t {
	int num_elements;
	int max_elements;
	dict_key_t *keys;
	dict_value_t *values;
} collision_bucket_t;

typedef union entry_t {
	collision_bucket_t *collision_buckets;
	dict_value_t *value;
} entry_t;

typedef struct dictionary_t {
	long num_collisions;
	long num_entries;
	long maximum_chain;
	long max_entries;
	dict_key_t *keys;
	entry_t *values;
	double load_factor;
} dictionary_t;

/*
 * Allocate a dictionary with a hash vector initialized to DICT_INITIAL_SIZE
 */
dictionary_t *
new_dictionary();

/*
 * Allocate a dictionary with a hash vector initialized to a user-defined value
 *
 * initial_size - this should be a prime number to help ensure good distribution
 */
dictionary_t *
new_dictionary_size(long initial_size);

/*
 * Allocate a dictionary with a hash vector initialized to a user-defined value
 *
 * initial_size - this should be a prime number to help ensure good distribution
 *
 * load_factor - between 0 and 1.0 to resize the dictionary when its size
 * 		exceeds this value * the current capacity of the dictionary
 */
dictionary_t *
new_dictionary_size_load(long initial_size, double load_factor);

/*
 * Free a dictionary created by new_dictionary()
 */
void
free_dictionary(dictionary_t *dict);

/*
 * Put a value into the dictionary
 *
 * dict - allocated by new_dictionary()
 * key - null-terminated string will be copied and managed by dictionary
 * value - void pointer (or 64-bit value) - must be managed by caller
 */
dict_value_t
dictionary_put(dictionary_t *dict, char *key, dict_value_t value);

/*
 * Retrieve a value from the dictionary
 *
 * dict - allocated by new_dictionary()
 * key - null-terminated string will be copied and managed by dictionary
 */
dict_value_t
dictionary_get(dictionary_t *dict, char *key);

/*
 * Remove an entry from the dictionary. The value at the key will be
 * returned.
 *
 * dict - allocated by new_dictionary()
 * key - null-terminated string will be copied and managed by dictionary
 */
dict_value_t
dictionary_remove(dictionary_t *dict, char *key);

/*
 * For each key/value pair in the dictionary, execute the enumeration function.
 *
 * dict - dictionary to enumerate
 * enum_function - function returning void that takes key, value as arguments
 */
void
dictionary_enumerate(dictionary_t *dict, dictionary_enumerator_t enum_function);

#endif
