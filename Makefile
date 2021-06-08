CC=gcc
CFLAGS=-Iinclude -Wall -Werror -g -Wno-unused 

SSRC=$(shell find src -name '*.c')
DEPS=$(shell find include -name '*.h')

LIBS=-pthread

all: server

setup:
	mkdir -p bin

server: setup $(DEPS)
	$(CC) $(CFLAGS) $(LIBS) $(SSRC) lib/protocol.o -o bin/zbid_server
	cp lib/zbid_client bin
	cp lib/auctionroom bin
	
.PHONY: clean

clean:
	rm -rf bin 
