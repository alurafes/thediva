CC = gcc
FLAGS = -Wall -Werror -ggdb -I../../  -I./include
LIBS = -lraylib

main: build/main.o build/dsc.o
	$(CC) $(FLAGS) -o build/main build/main.o build/dsc.o $(LIBS)

build/main.o: src/main.c
	$(CC) $(FLAGS) -c -o build/main.o src/main.c

build/dsc.o: src/dsc.c
	$(CC) $(FLAGS) -c -o build/dsc.o src/dsc.c