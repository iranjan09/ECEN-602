CC=gcc
CFLAGS=-c -Wall -Iinclude
header=include/common.h
SOURCES=$(wildcard src/*.c)
SRCDIR=src
OBJECT=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.c=.o))
OBJDIR=obj
serverDep=$(OBJDIR)/server.o $(OBJDIR)/common.o 
OUTPUTDIR=bin
SERVER=server
CLIENT=client
all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/common.o obj/server.o 

$(CLIENT): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/common.o obj/client.o -lpthread

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean 
clean:
	rm -f $(OBJDIR)/*.o $(OUTPUTDIR)/$(SERVER) $(OUTPUTDIR)/$(CLIENT)
