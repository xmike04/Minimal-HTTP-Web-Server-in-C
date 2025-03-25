

CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: wbserver

wbserver: wbserver.c
	$(CC) $(CFLAGS) -o wbserver wbserver.c

clean:
	rm -f wbserver