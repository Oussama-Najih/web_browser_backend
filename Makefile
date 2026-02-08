CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = browser.c server.c
OBJ = $(SRC:.c=.o)

all: server

server: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ljson-c

clean:
	rm -f $(OBJ) server
