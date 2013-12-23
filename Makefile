##########################################
# Makefile for hash
##########################################

INSTSET = 
INCPATH = b
LIBPATH = -lm
CC = gcc
CFLAGS = -g -std=gnu99 -Wall -D_GNU_SOURCE
#CFLAGS = -g -std=gnu99 -D_GNU_SOURCE -DDEBUG_VERBOSE_DICT_REMOVE
#CFLAGS = -g -std=gnu99 -D_GNU_SOURCE -DDEBUG_VERBOSE_DICT_PUT
#CFLAGS = -g -std=gnu99 -D_GNU_SOURCE -DDEBUG_VERBOSE_CB_UPDATE -DDEBUG_VERBOSE_DICT_ENUM
#CFLAGS = -g -std=c99 -D_POSIX_C_SOURCE
LINKOPTS = $(LIBPATH)

DEPENDENCIES = test_dictionary.c dictionary.c hash.c
OBJECTS = test_dictionary.o dictionary.o hash.o
TARGET = test_dictionary

all:	$(TARGET)

# Remove all objects and other temporary files.
objclean:
	rm -rf *.o 

# Remove all the libraries that were generated.
libclean:

# Remove all the executables.
execlean:
	rm -rf $(TARGET) test_hash bin core

# Remove all objects, libraries and executables along with other temporary files.
clean:	objclean libclean execlean

test_hash.o: test_hash.c hash.c

test_hash: test_hash.o hash.o
	$(CC) -o test_hash test_hash.o hash.o $(LINKOPTS)

test_dictionary: $(OBJECTS)
	$(CC) -o test_dictionary $(OBJECTS) $(LINKOPTS)

test_dictionary.o: test_dictionary.c $(DEPENDENCIES)

dictionary.o: dictionary.c $(DEPENDENCIES)
