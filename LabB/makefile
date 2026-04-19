# Lab B - Virus Detector & Anti-Virus
# Compiles AntiVirus.c into a 32-bit Linux executable

CC = gcc
# -m32: compile for 32-bit x86 architecture
# -Wall -Wextra: enable all common warnings
CFLAGS = -m32 -Wall -Wextra

all: AntiVirus

AntiVirus: AntiVirus.c
	$(CC) $(CFLAGS) -o AntiVirus AntiVirus.c

# Remove compiled binary
clean:
	rm -f AntiVirus

.PHONY: all clean
