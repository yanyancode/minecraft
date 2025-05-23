CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g
LDFLAGS=

SRCS=main.c player.c world.c network.c protocol.c
OBJS=$(SRCS:.c=.o)

TARGET=minecraft_server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
