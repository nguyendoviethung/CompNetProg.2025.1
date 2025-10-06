# Makefile - Bài tập 2
CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra

TARGET  := resolver
SRC     := resolver.c
OBJ     := $(SRC:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ) resolver.log
