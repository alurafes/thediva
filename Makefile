CC = gcc
FLAGS = -Wall -Werror -ggdb

thediva: build/thediva.o
	$(CC) $(FLAGS) -o build/thediva build/thediva.o

build/thediva.o: thediva.c
	$(CC) $(FLAGS) -c -o build/thediva.o thediva.c