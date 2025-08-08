# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS =

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)/objects
TEST_DIR = test
TEST_FILES_DIR = $(TEST_DIR)/test_files

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
MAIN_SRC = main.c
TEST_SRC = $(TEST_DIR)/test.c

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
MAIN_OBJ = $(BIN_DIR)/main.o
TEST_OBJ = $(TEST_DIR)/test.o

# Output executables
MAIN_EXEC = $(BIN_DIR)/http-server
TEST_EXEC = $(TEST_DIR)/http-server-test

# Default target
all: $(MAIN_EXEC)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(MAIN_EXEC): $(OBJS) $(MAIN_OBJ) | $(BIN_DIR)
	$(CC) $(OBJS) $(MAIN_OBJ) $(LDFLAGS) -o $@

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main.c
$(MAIN_OBJ): $(MAIN_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test.c
$(TEST_OBJ): $(TEST_SRC) | $(BIN_DIR)
	$(CC) -c $< -o $@

# Test target
test: $(TEST_EXEC)
	./$(TEST_EXEC)

# Link test executable
$(TEST_EXEC): $(TEST_OBJ) | $(TEST_DIR)
	$(CC) $(TEST_OBJ) -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR)/*.o $(MAIN_EXEC) $(TEST_EXEC) $(MAIN_OBJ) $(TEST_OBJ)

# Phony targets
.PHONY: all test clean
