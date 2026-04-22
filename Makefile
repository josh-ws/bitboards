CC=g++
CFLAGS=-O3 -march=native -g3 -march=native -Wall -Wextra --std=c++23
DEPENDS=
OUT=bitboards

all: Bitboards.cpp
	$(CC) $(DEPENDS) $(CFLAGS) Bitboards.cpp -o $(OUT)
