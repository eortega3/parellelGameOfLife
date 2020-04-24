CC = gcc
CFLAGS = -g -Wall -Werror -Wextra -pthread -std=gnu11


TARGETS = gol

all: $(TARGETS)


gol: gol.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGETS)

