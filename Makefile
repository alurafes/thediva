CC = g++
FLAGS = -Wall -std=c++20 -ggdb -Iinc

main: build/main.o build/dsc.o build/engine.o
	$(CC) $(FLAGS) -o build/main build/main.o build/dsc.o build/engine.o

build/engine.o: src/engine.cpp
	$(CC) $(FLAGS) -c -o build/engine.o src/engine.cpp

build/dsc.o: src/dsc/dsc.cpp
	$(CC) $(FLAGS) -c -o build/dsc.o src/dsc/dsc.cpp

build/main.o: src/main.cpp
	$(CC) $(FLAGS) -c -o build/main.o src/main.cpp
