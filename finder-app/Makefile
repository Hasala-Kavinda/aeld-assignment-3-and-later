CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -g -O2 -static
TARGET   = writer
SRCS     = writer.c
OBJS     = $(SRCS:.c=.o)

ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
