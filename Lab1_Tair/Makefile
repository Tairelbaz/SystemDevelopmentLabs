CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -std=c11
TARGET = count-words
MENU_TARGET = menu_map

all: $(TARGET) $(MENU_TARGET)

$(TARGET): count-words.c
	$(CC) $(CFLAGS) -o $(TARGET) count-words.c

$(MENU_TARGET): menu_map.c
	$(CC) $(CFLAGS) -o $(MENU_TARGET) menu_map.c

clean:
	rm -f $(TARGET) $(MENU_TARGET)