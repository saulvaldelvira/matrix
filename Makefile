.PHONY: install uninstall clean

CC = gcc
CCFLAGS = -O3
INSTALL_PATH ?= /usr/local

matrix: matrix.c console.h
	$(CC) matrix.c -o matrix $(CCFLAGS)

install: matrix
	@ sudo su -c '\
	  install -d $(INSTALL_PATH)/bin ; \
	  install -m 755 matrix $(INSTALL_PATH)/bin ; '

uninstall:
	@ sudo rm -f $(INSTALL_PATH)/bin/matrix

clean:
	@ rm -f matrix
