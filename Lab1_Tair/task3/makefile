CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -std=c11
TARGET = menu_map

all: $(TARGET)

$(TARGET): menu_map.c
	$(CC) $(CFLAGS) -o $(TARGET) menu_map.c

clean:
	rm -f $(TARGET)
