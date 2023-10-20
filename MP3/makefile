CC=gcc
CFLAGS=-c -Wall -Iinclude
header=include/include.h
SOURCES=$(wildcard src/*.c)
SRCDIR=src
OBJECT=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.c=.o))
OBJDIR=obj
serverDep=$(OBJDIR)/tftpserver.o  
OUTPUTDIR=bin
SERVER=tftpserver
all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/tftpserver.o 

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean 
clean:
	rm -f $(OBJDIR)/*.o $(OUTPUTDIR)/$(SERVER)
