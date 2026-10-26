
# make CC=arm-linux-gnueabihf-gcc

CC ?= gcc
CFLAGS ?= -Wall -Werror -g
TARGET = aesdsocket

all: $(TARGET)
default: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all default clean
