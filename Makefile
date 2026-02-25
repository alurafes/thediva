main: build/main.o
	g++ -Wall -std=c++20 -o build/main build/main.o

build/main.o: src/main.cpp
	g++ -std=c++20 -c -o build/main.o src/main.cpp