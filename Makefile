CC=clang++
CFLAGS=-O3 -g3 -march=native -mtune=native -Wall -Wextra --std=c++23 -fopenmp
DEPENDS=
OUT=bitboards

all: Bitboards.cpp
	$(CC) $(DEPENDS) $(CFLAGS) Bitboards.cpp -o $(OUT)
