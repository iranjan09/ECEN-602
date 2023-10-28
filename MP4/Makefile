CC=gcc
CFLAGS=-c -Wall -Iinclude
header=include/include.h
SOURCES=$(wildcard src/*.c)
SRCDIR=src
OBJECT=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.c=.o))
OBJDIR=obj
OUTPUTDIR=bin
SERVER=http
CLIENT=client
all:  $(CLIENT)


$(CLIENT): $(OBJECT)
	$(CC) -g -o $(OUTPUTDIR)/$@  obj/client.o 

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -g $(CFLAGS) -c $< -o $@

.PHONY: clean 
clean:
	rm -f $(OBJDIR)/*.o  $(OUTPUTDIR)/$(CLIENT)