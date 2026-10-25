CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -g -O2
TARGET   = writer
SRCS     = writer.c
OBJS     = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
