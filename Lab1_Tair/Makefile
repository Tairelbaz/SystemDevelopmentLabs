CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -std=c11
TARGET = count-words

all: $(TARGET)

$(TARGET): count-words.c
	$(CC) $(CFLAGS) -o $(TARGET) count-words.c

clean:
	rm -f $(TARGET)