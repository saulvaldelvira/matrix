.PHONY: install uninstall clean

CC = cc
CCFLAGS = -O3
INSTALL_PATH ?= /usr/local

matrix: matrix.c console.h
	$(CC) matrix.c -o matrix $(CCFLAGS)

install: matrix
	@ echo " matrix => $(INSTALL_PATH)/bin"
	@ install -d $(INSTALL_PATH)/bin
	@ install -m 755 matrix $(INSTALL_PATH)/bin

uninstall:
	@ echo " RM $(INSTALL_PATH)/bin/matrix"
	@ rm -f $(INSTALL_PATH)/bin/matrix

clean:
	@ rm -f matrix
