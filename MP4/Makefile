CC=gcc
CFLAGS=-c -Wall -Iinclude
header=include/include.h
SOURCES=$(wildcard src/*.c)
SRCDIR=src
OBJECT=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.c=.o))
OBJDIR=obj
serverDep=$(OBJDIR)/proxy.o 
OUTPUTDIR=bin
SERVER=proxy
CLIENT=client
all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/proxy.o -lpthread

$(CLIENT): $(OBJECT)
	$(CC) -g -o $(OUTPUTDIR)/$@ obj/client.o 

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -g $(CFLAGS) -c $< -o $@

.PHONY: clean 
clean:
	rm -f $(OBJDIR)/*.o $(OUTPUTDIR)/$(SERVER) $(OUTPUTDIR)/$(CLIENT)