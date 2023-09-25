CC = gcc
CFLAGS = -Wall -Iinclude
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
SERVER = echos
CLIENT = echo

# List of source files for the server and client
SERVER_SOURCES = $(SRC_DIR)/server.c $(SRC_DIR)/common.c
CLIENT_SOURCES = $(SRC_DIR)/client.c $(SRC_DIR)/common.c

SERVER_OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SOURCES))
CLIENT_OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SOURCES))

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^

$(CLIENT): $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/$(SERVER) $(BIN_DIR)/$(CLIENT)
