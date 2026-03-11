CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = browser/browser.c server.c state/state.c serialize/json-serialize.c
OBJ = $(SRC:.c=.o)

all: server

server: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ljson-c

clean:
	rm -f $(OBJ) server browser_state.json