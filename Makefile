CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I./include
LIB = -lsqlite3
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# Target executable
TARGET = $(BIN_DIR)/http_server

.PHONY: all clean

all: directories $(TARGET)

# Create directories if they don't exist
directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# Link object files to create executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install
install: all
	cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(notdir $(TARGET))