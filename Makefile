# Makefile for Network Monitoring and Visualization Tool
# Project: Network Monitor for Cisco Devices

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include
LDFLAGS = -lpthread

# Optional libraries (check if available)
# Uncomment these when libraries are installed:
# LDFLAGS += -lpcap -lncurses -lnetsnmp

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = include
TEST_DIR = tests
OBJ_DIR = $(BUILD_DIR)/obj

# Target executable
TARGET = $(BIN_DIR)/netmon
TEST_TARGET = $(BIN_DIR)/test_runner

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(wildcard $(SRC_DIR)/network/*.c) \
          $(wildcard $(SRC_DIR)/monitoring/*.c) \
          $(wildcard $(SRC_DIR)/visualization/*.c) \
          $(wildcard $(SRC_DIR)/utils/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Test sources
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(OBJ_DIR)/test/%.o)

# Debug flags
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Default target
.PHONY: all
all: $(TARGET)

# Create necessary directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/network
	@mkdir -p $(OBJ_DIR)/monitoring
	@mkdir -p $(OBJ_DIR)/visualization
	@mkdir -p $(OBJ_DIR)/utils
	@mkdir -p $(OBJ_DIR)/test

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Build main executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "Debug build complete"

# Release build
.PHONY: release
release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)
	@echo "Release build complete"

# Build tests
.PHONY: test
test: $(TEST_TARGET)
	@echo "Running tests..."
	@$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS)) | $(BIN_DIR)
	@echo "Linking test runner..."
	$(CC) $(TEST_OBJECTS) $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS)) -o $(TEST_TARGET) $(LDFLAGS)

$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling test $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
.PHONY: run
run: $(TARGET)
	@echo "Running $(TARGET)..."
	@$(TARGET)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Install to system (requires sudo)
.PHONY: install
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@install -m 755 $(TARGET) /usr/local/bin/netmon
	@mkdir -p /etc/netmon
	@install -m 644 configs/devices.conf /etc/netmon/devices.conf
	@echo "Installation complete"

# Uninstall from system (requires sudo)
.PHONY: uninstall
uninstall:
	@echo "Uninstalling netmon..."
	@rm -f /usr/local/bin/netmon
	@rm -rf /etc/netmon
	@echo "Uninstall complete"

# Format code (requires clang-format)
.PHONY: format
format:
	@echo "Formatting code..."
	@find $(SRC_DIR) $(INCLUDE_DIR) -name '*.c' -o -name '*.h' | xargs clang-format -i
	@echo "Format complete"

# Static analysis (requires cppcheck)
.PHONY: check
check:
	@echo "Running static analysis..."
	@cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_DIR) $(INCLUDE_DIR)

# Show help
.PHONY: help
help:
	@echo "Network Monitoring Tool - Makefile targets:"
	@echo ""
	@echo "  make              - Build the project (default)"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make release      - Build optimized release version"
	@echo "  make test         - Build and run tests"
	@echo "  make run          - Build and run the application"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install to /usr/local/bin (requires sudo)"
	@echo "  make uninstall    - Remove from system (requires sudo)"
	@echo "  make format       - Format code with clang-format"
	@echo "  make check        - Run static analysis with cppcheck"
	@echo "  make help         - Show this help message"
	@echo ""

# Dependencies
-include $(OBJECTS:.o=.d)
-include $(TEST_OBJECTS:.o=.d)
