Dictionary
======

A dictionary is one of the most important and commonly used programming concepts, and in the past few
years has become an essential language feature, rather than a separate tool that to be obtained from
an external code library.

The implementation provided here is a simple hash table with no support for concurrent access. The only keys supported are 8-bit character strings, and the values that may be added to the table are any value that can be cast to an 64-bit pointer.

This is a work in progress and there are a lot of obvious things that haven't been done yet:
* support for a 32-bit compiler
* support for Unicode keys
* support for C11 language features (in particular, Unicode strings)
* support for autoconf or cmake or something to save me from minor Makefile hell

I've tested it with gcc 4.2 on Linux, FreeBSD 9.2 and MinGW on Windows, and clang 3.3 on Linux, Windows, FreeBSD and OS X

Usage
-----

1. Creation and deletion

There are 3 different ways to create a dictionary, depending on how much data 

```C
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
```

Enumeration
-----

One of the interesting aspects of this implementation  


Testing
-----

To test performance and hashing quality I used the (2011) Word frequency data from the Corpus of Contemporary American
English (COCA), downloaded from http://www.wordfrequency.info on January 15, 2011



