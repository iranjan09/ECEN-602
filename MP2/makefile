CC=gcc
CFLAGS= -Wall -Iinclude

SRCDIR=src
OBJDIR=obj
OUTPUTDIR=bin

SERVER=server
CLIENT=client

SOURCES=$(wildcard src/*.c)
OBJECT=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.c=.o))

all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/server.o 

$(CLIENT): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/client.o -lpthread

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean 
clean:
	rm -f $(OBJDIR)/*.o $(OUTPUTDIR)/$(SERVER) $(OUTPUTDIR)/$(CLIENT)
