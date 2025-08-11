# logmgrd 项目 Makefile

CC      := gcc
CFLAGS  := -Wall -O2 -pthread
TARGET  := logmgrd
PREFIX  := /usr/local

SRCDIR  := src
OBJDIR  := build

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/logmgrd.o: $(SRCDIR)/logmgrd.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(TARGET) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/etc
	cp logmgrd.conf $(DESTDIR)$(PREFIX)/etc/logmgrd.conf

.PHONY: all clean install