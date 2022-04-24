CC = x86_64-w64-mingw32-gcc

all: main

main: main.c
	$(CC) -o colour-tiles.exe main.c -lgdi32