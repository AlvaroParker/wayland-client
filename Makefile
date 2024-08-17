CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lwayland-client -lrt
OUTPUT = client
SRCS = xdg-shell-protocol.c wayland-client.c pool.c

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(OUTPUT) $(LIBS)
