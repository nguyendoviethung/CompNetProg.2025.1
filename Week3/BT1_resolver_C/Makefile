# Makefile - BT1
CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra

TARGET := resolver
SRC := resolver.c
OBJ := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
