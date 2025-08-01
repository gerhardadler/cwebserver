CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude
SRC = $(wildcard src/*.c)
OUT = webserver

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OUT)
