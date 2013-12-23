/*
 * dictionary.c
 *
 * 2013-12-16 Steven Wart created this file
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dictionary.h"
#include "hash.h"

#define CB_INITIAL_SIZE 16
#define MAX_KEY 4096

/* ---------- private declarations ---------- */

/*
 * Free the private dictionary structures without freeing the public dictionary structure
 */
void
dictionary_free_internal(dictionary_t *dict);

/*
 * Resize the dictionary, rehashing all keys
 * 
 * Called by dictionary_put() - not safe for concurrent updates
 *
 * dict- dictionary to resize
 * new_size - new size - this should be prime
 */
void
dictionary_rebuild_table(dictionary_t *dict, long new_size);

/*
 * Retrieve a value from a collision bucket
 *
 * bucket - allocated by new_collision_bucket()
 * key - null-terminated string will be copied and managed by collision bucket
 */
dict_value_t
collision_bucket_get(collision_bucket_t *bucket, dict_key_t key);

/*
 * Allocate or reallocate buckets to handle increases in the size of the collision buckets
 *
 * The initial size of the collision bucket is 16 elements (CB_INITIAL_SIZE)
 * It will grow by 8 elements on subsequent invocations from collision_bucket_addpair()
 *
 * bucket - collision bucket allocated by new_collision_bucket()
 * new_size - size in bytes of key/value arrays
 */
void
cb_reinitializeArrays(collision_bucket_t *bucket, int new_size);

/*
 * Stores the key/value pair in the collision bucket
 *
 * Return 1 if this operation added a new key, 0 if it replaced the value of an existing key
 *
 * bucket - allocated by new_collision_bucket()
 * key - null-terminated string will be copied and managed by collision bucket
 * value - void pointer (or 64-bit value) - must be managed by caller
 */
int
collision_bucket_addpair(collision_bucket_t *bucket, char *key, dict_value_t value);

/*
 * Retrieve the size of a collision bucket
 *
 * bucket - allocated by new_collision_bucket()
 */
int
collision_bucket_size(collision_bucket_t *bucket);

/*
 * Allocate a collision bucket, key/value vectors are initialized lazily
 */
collision_bucket_t *
new_collision_bucket();

/*
 * Free a collision bucket obtained from new_collision_bucket()
 */
void
free_collision_bucket(collision_bucket_t *bucket);

/*
 * Returns a prime number that is greater than or equal to intValue.
 * The prime number is not necessarily the smallest prime number that is
 * greater than or equal to intValue.  For intValue greater than our
 * highest selected prime, returns an odd number.
 */
long
select_next_prime(int intValue);

/*
 * Debugging utility - prints the keys as strings, and the pointer addresses of the values
 */
void
print_collision_buckets(dictionary_t *dict);


/* ---------- public definitions ---------- */

/*
 * Allocate a dictionary with a hash vector initialized to DICT_INITIAL_SIZE
 */
dictionary_t *
new_dictionary()
{
	return new_dictionary_size(DICT_INITIAL_SIZE);
}

/*
 * Allocate a dictionary with a hash vector initialized to a user-defined value
 *
 * initial_size - should be a power of 2 because the hash function uses
 * 		one less than this value as a bit mask to calculate the modulus
 */
dictionary_t *
new_dictionary_size(long initial_size)
{
	return new_dictionary_size_load(initial_size, LOAD_FACTOR);
}

/*
 * Allocate a dictionary with a hash vector initialized to a user-defined value
 *
 * initial_size - must be a power of 2 because the hash function uses
 * 		one less than this value as a bit mask to calculate the modulus
 *
 * load_factor - between 0 and 1.0 to resize the dictionary when its size
 * 		exceeds this value * the current capacity of the dictionary
 */
dictionary_t *
new_dictionary_size_load(long initial_size, double load_factor)
{
	dictionary_t *dict = calloc(1, sizeof(dictionary_t));

	dict->load_factor = load_factor;

	dict->max_entries = initial_size;
	dict->keys = (dict_key_t *)calloc(initial_size, sizeof(dict_key_t));
	dict->values = (entry_t *)calloc(initial_size, sizeof(entry_t));

	return dict;
}

/*
 * Free a dictionary created by new_dictionary()
 */
void
free_dictionary(dictionary_t *dict)
{
	dictionary_free_internal(dict);
	free(dict);
}

/*
 * Put a value into the dictionary
 *
 * dict - allocated by new_dictionary()
 * key - null-terminated string will be copied and managed by dictionary
 * value - void pointer (or 64-bit value) - must be managed by caller
 */
dict_value_t
dictionary_put(dictionary_t *dict, char *key, dict_value_t value)
{
	if (key == NULL)
		return NULL;

	if ((dict->keys == NULL) || (dict->values == NULL)) {
		fprintf(stderr, "Attempt to use uninitialized dictionary %p\n", dict);
		return NULL;
	}

	dict_value_t previous = dictionary_get(dict, key);
	long theHash = hash((unsigned char *)key) % (dict->max_entries-1);

	collision_bucket_t *bucket;
	dict_key_t hash_key;

#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_put() key '%s' hash=%lu\n", key, theHash);
#endif

	// the entry for the hash code will be null if
	// (a) there is no entry for that key or
	// (b) there is an entry for that key with a collision bucket
	if ((hash_key = dict->keys[theHash]) == NULL) {
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_put() no entry for key '%s' hash=%lu\n", key, theHash);
#endif
		entry_t entry = dict->values[theHash];
		if ((bucket = entry.collision_buckets) == NULL) {
#ifdef DEBUG_VERBOSE_DICT_PUT
			printf("dictionary_put() new entry, %zu bytes\n", strlen(key) + 1);
#endif
			dict_key_t new_string = (dict_key_t)calloc(1, strlen(key) + 1);
			dict->keys[theHash] = new_string;
			// new entry
			strcpy(new_string, key);
			dict->values[theHash].value = value;
			dict->num_entries++;
#ifdef DEBUG_VERBOSE_DICT_PUT
			printf("dictionary_put() new entry, '%s' (%p) has value %p\n",
				new_string, new_string, dict->values[theHash].value);
#endif
		}
		// if the key is null, then the value is a collision bucket
		else {
#ifdef DEBUG_VERBOSE_DICT_PUT
			printf("dictionary_put() key '%s' hash=%lu\n", key, theHash);
#endif
			if (collision_bucket_addpair(bucket, key, value) > 0) {
#ifdef DEBUG_VERBOSE_DICT_PUT
				printf("dictionary_put() added new entry to existing bucket: ");
#endif
				dict->num_entries++;
				if (collision_bucket_size(bucket) > 1) {
					dict->num_collisions++;
				}
				if (collision_bucket_size(bucket) > dict->maximum_chain)
					dict->maximum_chain = collision_bucket_size(bucket);
			}
			else {
#ifdef DEBUG_VERBOSE_DICT_PUT
				printf("dictionary_put() replaced existing entry in bucket: ");
#endif
			}
			for (int j=0; j < bucket->num_elements; j++) {
#ifdef DEBUG_VERBOSE_DICT_PUT
				printf("%s:%lu (%p) ", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
#endif
			}
#ifdef DEBUG_VERBOSE_DICT_PUT
			printf("\n");
#endif
		}
	}
	// replace existing value
	else if (strncmp(hash_key, key, MAX_KEY) == 0) {
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_put() replace existing value at key '%s' with value '%p'\n", hash_key, value);
#endif
		dict->values[theHash].value = value;
	}
	// otherwise add a new value to the dictionary
	else {
		dict_value_t hash_value = dict->values[theHash].value;
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_put() collision: new key '%s' hashes to %lu, value=%lu (%p)",
			key, theHash, (long)value, value);

		printf(" (existing key='%s', value=%lu (%p))\n", dict->keys[theHash], (long)hash_value, hash_value);
#endif
		bucket = new_collision_bucket();
		// remove reference to hash_key
		dict->keys[theHash] = NULL;
		dict->values[theHash].collision_buckets = bucket;
		
		collision_bucket_addpair(bucket, key, value);
		dict->num_entries++;

		int num_added = collision_bucket_addpair(bucket, hash_key, hash_value);
		// hash_key has been reallocated and has no more references, goodbye!
		free(hash_key);

		if (num_added > 0) {
			if (collision_bucket_size(bucket) > 1) {
				dict->num_collisions++;
			}
			if (collision_bucket_size(bucket) > dict->maximum_chain) {
				dict->maximum_chain = collision_bucket_size(bucket);
			}
		}
	}

	if (dict->num_entries > dict->load_factor * dict->max_entries) {
		long new_size = select_next_prime(dict->num_entries * 2);
		dictionary_rebuild_table(dict, new_size);
	}

#ifdef DEBUG_VERBOSE_DICT_PUT
	printf("dictionary_put() returning previous value %p\n", previous);
#endif
	return previous;
}

/*
 * Retrieve a value from the dictionary
 *
 * dict - allocated by new_dictionary()
 * key - null-terminated string will be copied and managed by dictionary
 */
dict_value_t
dictionary_get(dictionary_t *dict, char *key)
{
	if (key == NULL)
		return NULL;

	long hash_value = hash((unsigned char *)key) % (dict->max_entries-1);
#ifdef DEBUG_VERBOSE_DICT_PUT
	printf("dictionary_get() looking for key '%s' at index %lu of dict-keys = %p\n", key, hash_value, dict->keys);
#endif
	dict_key_t hash_key = dict->keys[hash_value];
	if (hash_key == 0) {
		// null key means we may have a collision bucket
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_get() null key means we may have a collision bucket\n");
#endif
		entry_t entry = dict->values[hash_value];
		if (entry.collision_buckets != NULL) {
			return collision_bucket_get(entry.collision_buckets, key);
		}
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("entry.collision_buckets is null\n");
#endif
	}
	else if (strncmp(key, hash_key, MAX_KEY) == 0) {
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_get() found '%s', about to retrieve value\n", key);
#endif
		entry_t entry = dict->values[hash_value];
#ifdef DEBUG_VERBOSE_DICT_PUT
		printf("dictionary_get() found ['%s':%lu]\n", key, (long)entry.value);
#endif
		return entry.value;
	}
#ifdef DEBUG_VERBOSE_DICT_PUT
	printf("dictionary_get() key not found '%s'\n", key);
#endif
	return NULL;
}

/*
 * Remove an entry from the dictionary. The value at the key will be
 * returned.
 *
 * dict - allocated by new_dictionary()
 * key - null-terminated string will be copied and managed by dictionary
 */
dict_value_t
dictionary_remove(dictionary_t *dict, char *key_in)
{
	if (key_in == NULL)
		return NULL;

	dict_value_t value = NULL;
	long hash_index = hash((unsigned char *)key_in) % (dict->max_entries-1);

	dict_key_t key = dict->keys[hash_index];
	if (key == NULL) {
		entry_t entry = dict->values[hash_index];
		// we may have a collision bucket
		if (entry.value != NULL) {
			collision_bucket_t *bucket = entry.collision_buckets;
			int num_buckets = bucket->num_elements;
			for (int j=0; j < num_buckets; j++) {
				key = bucket->keys[j];
				if ((key != NULL) && (strcmp(key_in, key) == 0)) {
					value = bucket->values[j];
					bucket->values[j] = NULL;
					free(bucket->keys[j]);
					bucket->keys[j] = NULL;
					dict->num_collisions--;
					int new_size = bucket->num_elements-1;

					// if there is only one element remaining in this bucket, get rid of it
					if (new_size == 1) {
						int last_el_index = (j == 0) ? 1 : 0;
#ifdef DEBUG_VERBOSE_DICT_REMOVE							
						printf("dictionary_remove() [bucket] promoting last bucket element [%s:%lu (%p)] to value slot %lu\n",
							bucket->keys[last_el_index], (long)bucket->values[last_el_index], (void *)bucket->values[last_el_index], hash_index);
#endif
						dict->keys[hash_index] = bucket->keys[last_el_index];
						dict->values[hash_index].value = bucket->values[last_el_index];
						bucket->keys[last_el_index] = NULL;
						bucket->values[last_el_index] = NULL;
						free(bucket->keys);
						free(bucket->values);
						free(bucket);
						break;
					}
					// otherwise move the last element in this bucket to the current slot
					else {
#ifdef DEBUG_VERBOSE_DICT_REMOVE							
						dict_value_t last_element = bucket->values[new_size];
						printf("dictionary_remove() moving last element [%s:%lu (%p)] to vacated slot index %d (new size=%d)\n", 
							(char *)bucket->keys[new_size], (long)last_element, (void *)last_element, j, new_size);
#endif
						bucket->keys[j] = bucket->keys[new_size];
						bucket->values[j] = bucket->values[new_size];
						bucket->keys[new_size] = NULL;
						bucket->values[new_size] = NULL;
						bucket->num_elements = new_size;
					}
#ifdef DEBUG_VERBOSE_DICT_REMOVE							
					printf("dictionary_remove() [bucket] removed key='%s', value=%lu (%p)\n", key_in, (long)value, (void *)value);
					print_collision_buckets(dict);
#endif
					break;
				}
			}
		}
	}
	else if (strcmp(key_in, key) == 0) {
		value = dict->values[hash_index].value;
		dict->values[hash_index].value = NULL;	// ensure we don't mistake it for a bucket
		free(dict->keys[hash_index]);
		dict->keys[hash_index] = NULL;			// this key no longer exists
		dict->num_entries--;
#ifdef DEBUG_VERBOSE_DICT_REMOVE							
		printf("dictionary_remove() [values] removed key='%s', value=%lu (%p)\n", key_in, (long)value, (void *)value);
		print_collision_buckets(dict);
#endif
	}

	return value;
}

/*
 * For each key/value pair in the dictionary, execute the enumeration function.
 *
 * dict - dictionary to enumerate
 * enum_function - function returning void that takes key, value as arguments
 */
void
dictionary_enumerate(dictionary_t *dict, dictionary_enumerator_t enum_function)
{
	for (int i=0; i < dict->max_entries; i++) {
		dict_key_t key = dict->keys[i];
		if (key == NULL) {
			entry_t entry = dict->values[i];
			// we may have a collision bucket
			if (entry.value != NULL) {
				collision_bucket_t *bucket = entry.collision_buckets;
				for (int j=0; j < bucket->num_elements; j++) {
					key = bucket->keys[j];
					if (key != NULL) {
						dict_value_t value = bucket->values[j];
#ifdef DEBUG_VERBOSE_DICT_ENUM
						printf("dictionary_enumerate() [bucket] calling enum_function() with key='%s', value=%lu (%p)\n",
							key, (long)value, (void *)value);
#endif
						enum_function(key, value);
					}
				}
			}
		}
		else {
			dict_value_t value = dict->values[i].value;
#ifdef DEBUG_VERBOSE_DICT_ENUM
			printf("dictionary_enumerate() [values] calling enum_function() with key='%s', value=%lu (%p)\n",
				key, (long)value, (void *)value);
#endif
			enum_function(key, value);
		}
	}
}

/* --- private functions --- */

/*
 * Free the private dictionary structures without freeing the public dictionary structure
 */
void
dictionary_free_internal(dictionary_t *dict)
{
	if (dict->values) {
		for (int i=0; i < dict->max_entries; i++) {
			if (dict->keys[i] == NULL) {
				// then the value is a collision bucket (or null)
				collision_bucket_t *bucket = dict->values[i].collision_buckets;
				if (bucket == NULL) {
					continue;
				}
#ifdef DEBUG_VERBOSE_DICT_INIT
				printf("free_dictionary() freeing collision bucket %d (%p)\n", i, bucket);
#endif
				free_collision_bucket(bucket);
			}
		}
		free(dict->values);
	}

	if (dict->keys) {
		// we reallocate the keys, but not the values
		for (int i=0; i < dict->max_entries; i++) {
			free(dict->keys[i]);
		}
		free(dict->keys);
	}
}

/*
 * Resize the dictionary, rehashing all keys
 * 
 * Called by dictionary_put() - not safe for concurrent updates
 *
 * dict- dictionary to resize
 * new_size - new size - this should be prime
 */
void
dictionary_rebuild_table(dictionary_t *dict, long new_size)
{
#ifdef DEBUG_VERBOSE_DICT_RESIZE
	printf("Resizing dictionary from %lu to %lu (%lu entries, %lu collisions)\n",
			dict->max_entries, new_size, dict->num_entries, dict->num_collisions);
	// printf("Collision buckets before resize:\n");
	// print_collision_buckets(dict);
#endif
	dictionary_t *new_dict = new_dictionary_size(new_size);

	void
	copy_pair(dict_key_t key, dict_value_t value)
	{
		dictionary_put(new_dict, key, value);
	}

	dictionary_enumerate(dict, &copy_pair);

	if (dict->num_entries != new_dict->num_entries) {
		fprintf(stderr, "Old dictionary entries %lu does not match new dictionary %lu\n",
			dict->num_entries, new_dict->num_entries);
	}

	// we have no GCto keep track of all the collision buckets, just start over
	dictionary_free_internal(dict);

	dict->keys = new_dict->keys;
	dict->values = new_dict->values;
	dict->max_entries = new_dict->max_entries;
	dict->num_collisions = new_dict->num_collisions;
	dict->maximum_chain = new_dict->maximum_chain;

#ifdef DEBUG_VERBOSE_DICT_RESIZE
	// printf("Collision buckets after resize:\n");
	// print_collision_buckets(dict);
#endif
	free(new_dict);
}

/*
 * Retrieve a value from a collision bucket
 *
 * bucket - allocated by new_collision_bucket()
 * key - null-terminated string will be copied and managed by collision bucket
 */
dict_value_t
collision_bucket_get(collision_bucket_t *bucket, dict_key_t key)
{
	if (key == NULL)
		return NULL;

#ifdef DEBUG_VERBOSE_CB_UPDATE
	printf("collision_bucket_get() before get has %d entries:\n", bucket->num_elements);
	for (int j=0; j < bucket->num_elements; j++) {
		printf("\t%s:%lu (%p)\n", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
	}
#endif
	for (int i=0; i < bucket->num_elements; i++) {
		if (strncmp(key, bucket->keys[i], MAX_KEY) == 0) {
			return bucket->values[i];
		}
	}
	return NULL;
}

/*
 * Allocate or reallocate buckets to handle increases in the size of the collision buckets
 *
 * The initial size of the collision bucket is 16 elements (CB_INITIAL_SIZE)
 * It will grow by 8 elements on subsequent invocations from collision_bucket_addpair()
 *
 * bucket - collision bucket allocated by new_collision_bucket()
 * new_size - size in bytes of key/value arrays
 */
void
cb_reinitializeArrays(collision_bucket_t *bucket, int new_size)
{
#ifdef DEBUG_VERBOSE_CB_INIT
	printf("cb_reinitializeArrays() before initialization has %d entries:\n", bucket->num_elements);
	for (int j=0; j < bucket->num_elements; j++) {
		printf("\t%s:%lu (%p)\n", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
	}
#endif
	dict_key_t *new_keys = (dict_key_t *)calloc(1, sizeof(dict_key_t) * new_size);
	dict_value_t *new_values = (dict_value_t *)calloc(1, sizeof(dict_value_t) * new_size);
	if (bucket->keys != NULL) {
		for (int i=0; i < bucket->num_elements; i++) {
			new_keys[i] = bucket->keys[i];
			new_values[i] = bucket->values[i];
		}
		free(bucket->keys);
		free(bucket->values);
	}
	bucket->keys = new_keys;
	bucket->values = new_values;
	bucket->max_elements = new_size;
#ifdef DEBUG_VERBOSE_CB_INIT
	printf("cb_reinitializeArrays() after initialization has %d entries:\n", bucket->num_elements);
	for (int j=0; j < bucket->num_elements; j++) {
		printf("\t%s:%lu (%p)\n", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
	}
#endif
}

/*
 * Stores the key/value pair in the collision bucket
 *
 * Return 1 if this operation added a new key, 0 if it replaced the value of an existing key
 *
 * bucket - allocated by new_collision_bucket()
 * key - null-terminated string will be copied and managed by collision bucket
 * value - void pointer (or 64-bit value) - must be managed by caller
 */
int
collision_bucket_addpair(collision_bucket_t *bucket, char *key, dict_value_t value)
{
	if (key == NULL) {
		fprintf(stderr, "collision_bucket_addpair() NULL key used for collision bucket\n");
		return -1;
	}

#ifdef DEBUG_VERBOSE_CB_UPDATE
	printf("collision_bucket_addpair() before adding ['%s':%lu] has %d entries:\n", key, (long)value, bucket->num_elements);
	for (int j=0; j < bucket->num_elements; j++) {
		printf("\t%s:%lu (%p)\n", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
	}
#endif
	if ((bucket->keys == NULL) || (bucket->values == NULL)) {
#ifdef DEBUG_VERBOSE_CB_UPDATE
		printf("collision_bucket_addpair() reinitializing cb arrays\n");
#endif
		cb_reinitializeArrays(bucket, CB_INITIAL_SIZE);
	}

	for (int i=0; i < bucket->num_elements; i++) {
#ifdef DEBUG_VERBOSE_CB_UPDATE
		printf("collision_bucket_addpair() comparing '%s' to '%s'\n", bucket->keys[i], key);
#endif
		if (strncmp(bucket->keys[i], key, MAX_KEY) == 0) {
			// replace an entry
			bucket->values[i] = value;	// caller must have reference to old value to return to client
			return 0;
		}
	}
	if (bucket->num_elements >= bucket->max_elements) {
		cb_reinitializeArrays(bucket, bucket->num_elements + 8);
	}

	// adding a new key, need to allocate a string touse
	dict_key_t new_key = (dict_key_t)calloc(1, strlen(key) + 1);
	strcpy(new_key, key);

	bucket->keys[bucket->num_elements] = new_key;
	bucket->values[bucket->num_elements] = value;
	bucket->num_elements++;
#ifdef DEBUG_VERBOSE_CB_UPDATE
	printf("collision_bucket_addpair() after appending new element, now has %d entries:\n", bucket->num_elements);
	for (int j=0; j < bucket->num_elements; j++) {
		printf("\t%s:%lu (%p)\n", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
	}
#endif
	return 1;
}

/*
 * Retrieve the size of a collision bucket
 *
 * bucket - allocated by new_collision_bucket()
 */
int
collision_bucket_size(collision_bucket_t *bucket)
{
	return bucket->num_elements;
}

/*
 * Allocate a collision bucket, key/value vectors are initialized lazily
 */
collision_bucket_t *
new_collision_bucket()
{
	return (collision_bucket_t *)calloc(1, sizeof(collision_bucket_t));
}

/*
 * Free a collision bucket obtained from new_collision_bucket()
 */
void
free_collision_bucket(collision_bucket_t *bucket)
{
#ifdef DEBUG_VERBOSE_CB_INIT
	printf("free_collision_bucket() freeing %d elements from %p\n", bucket->num_elements, bucket);
#endif
	for (int i=0; i < bucket->num_elements; i++) {

#ifdef DEBUG_VERBOSE_CB_INIT
		printf("free_collision_bucket() freeing string %s\n", bucket->keys[i]);
#endif
		free(bucket->keys[i]);
		bucket->keys[i] = NULL;
		// bucket values are managed by the client
		bucket->values[i] = NULL;
	}
	free(bucket->keys);
	free(bucket->values);
	free(bucket);
}

/*
 * Returns a prime number that is greater than or equal to intValue.
 * The prime number is not necessarily the smallest prime number that is
 * greater than or equal to intValue.  For intValue greater than our
 * highest selected prime, returns an odd number.
 */
long
select_next_prime(int intValue)
{
	long primes[148] = {
		3, 5, 7, 11, 17, 19, 23, 29, 37, 53, 73, 107, 157, 233, 347, 503, 751,
		1009, 1511, 2003, 3001, 4001, 5003, 6007, 7001, 8009, 9001,
		10007, 11003, 12007, 13001, 14009, 15013, 16001, 17011, 18013, 19001,
		20011, 21001, 22003, 23003, 24001, 25013, 26003, 27011, 28001, 29009,
		30011, 31013, 32003, 33013, 34019, 35023, 36007, 37003, 38011, 39019,
		40009, 41011, 42013, 43003, 44017, 45007, 46021, 47017, 48017, 49003,
		50021, 51001, 52009, 53003, 54001, 55001, 56003, 57037, 58013, 59009,
		60013, 61001, 62003, 63029, 64007, 65003, 66029, 67003, 68023, 69001,
		70001, 71011, 72019, 73009, 74017, 75011, 76001, 77003, 78007, 79031,
		80021, 81001, 82003, 83003, 84011, 85009, 86011, 87011, 88001, 89003,
		90001, 91009, 92003, 93001, 94007, 95003, 96001, 97001, 98009, 99013,
		100003, 101009, 102001, 103001, 104003, 224737, 350377, 479909,
		611953, 746773, 882377, 1020379, 1159523, 1299709, 2750159, 4256233,
		5800079, 7368787, 8960453, 10570841, 12195257, 13834103, 15485863,
		32452843, 49979687, 67867967, 86028121, 104395301, 122949823,
		141650939, 160481183 };

	int high = 147;	// primes size - 1

	// intValue greater than highest selected prime returns an odd number
	if (intValue > primes[high]) {
		return intValue * 2 + 1;
	}

	int low = 0;

	// Binary search for closest selected prime that is >= intValue

	while (high - low > 1) {
		int mid = (high + low) / 2;
		long prime = primes[mid];
		if (intValue <= prime)
			high = mid;
		else
			low = mid;
	}

	return primes[high];
}

void
print_collision_buckets(dictionary_t *dict)
{
	for (int i=0; i < dict->max_entries; i++) {
		if (dict->keys[i] == NULL) {
			entry_t entry = dict->values[i];
			if (entry.value != NULL) {
				collision_bucket_t *bucket = entry.collision_buckets;
				printf("bucket %i has %d entries:\n", i, bucket->num_elements);
				for (int j=0; j < bucket->num_elements; j++) {
					printf("\t%s:%lu (%p)\n", bucket->keys[j], (long)bucket->values[j], bucket->values[j]);
				}
			}
		}
	}
}

