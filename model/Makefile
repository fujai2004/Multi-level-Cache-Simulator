#
# Student makefile for Cache Lab
# Note: requires a 64-bit x86-64 system 
#
CC = gcc
CFLAGS = -g -Wall  -std=gnu99 -m64

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
	CFLAGS += -static
endif

all: cache 2level-mutex 2level

cache: cache.c main.c
	$(CC) $(CFLAGS) -o $@ cache.c main.c -lm 

2level: cache.c 2level-main.c
	$(CC) $(CFLAGS) -o $@ cache.c 2level-main.c -lm  

2level-mutex: cache.c 2level-mutex-main.c
	$(CC) $(CFLAGS) -o $@ cache.c 2level-mutex-main.c -lm  
		
#	-static

#
# Clean the src dirctory
#
clean:
	rm -rf *.o
	rm -f 2level
	rm -f 2level-mutex
	rm -f cache
	rm -f trace.all trace.f*
	rm -f .csim_results .marker
