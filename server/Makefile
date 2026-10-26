
# make CC=arm-linux-gnueabihf-gcc

CC ?= gcc
CFLAGS ?= -Wall -Werror -g -O2
TARGET = aesdsocket

all: $(TARGET)
default: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all default clean
