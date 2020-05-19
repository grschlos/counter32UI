CC = gcc

default: all

all: clean libcounter.a listener

libcounter.a: counter.o
	ar rcs $@ $^

counter.o: counter.c counter.h
	$(CC) -Wall -ggdb -c $<

listener: listener.c
	$(CC) -Wall -pthread -ggdb -o listen $< -L. -lcounter

clean:
	$(RM) *.o *.a listen

