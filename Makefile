

CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: webserver

webserver: webserver.c
	$(CC) $(CFLAGS) -o webserver webserver.c

clean:
	rm -f webserver