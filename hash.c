/*
 * djb2
 * 
 * this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c.
 * another version of this algorithm (now favored by bernstein) uses xor:
 * 	hash(i) = hash(i - 1) * 33 ^ str[i];
 * the magic of number 33 (why it works better than many other constants, prime or not)
 * has never been adequately explained.
 *
 * 2013-12-18 Steven Wart added modulus argument to prevent overflow problems
 *
 * the GemStone KeyValueDictionary has a funky algorithm to regrow itself if the collision limit is too high
 * the initial collision limit is 500 * the initial size of the dictionary
 * when it resizes it chooses the next value from a hard-coded collection of prime numbers
 *
 * The problem with bit-masking at each iteration of the loop below is we are throwing away
 * most of the bits in the hash. Unless the modulus is a bitmask the performance of this hash
 * will be very poor.
 *
 * For an extreme example, adding 474000 words to a dictionary that defaults to 16 buckets is very slow 
 *
 * http://stackoverflow.com/questions/9413966/why-initialcapacity-of-hashtable-is-11-while-the-default-initial-capacity-in-has
 */

#include <limits.h>
#include "hash.h"

unsigned long
hash(unsigned char *str)
{
    unsigned long hash = 5381;
    long mask = LONG_MAX; // 0x7fffffffffffffff or 4026531839

    int c;

    while ((c = *str++))
        hash = (((hash << 5) + hash) + c) & mask; /* hash * 33 + c */

    return hash;
}

