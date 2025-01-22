.PHONY: install uninstall clean

CC = cc
CCFLAGS = -O3
PREFIX ?= /usr/local

matrix: matrix.c console.h
	$(CC) matrix.c -o matrix $(CCFLAGS)

install: matrix
	@ echo " matrix => $(PREFIX)/bin"
	@ install -d $(PREFIX)/bin
	@ install -m 755 matrix $(PREFIX)/bin

uninstall:
	@ echo " RM $(PREFIX)/bin/matrix"
	@ rm -f $(PREFIX)/bin/matrix

clean:
	@ rm -f matrix
