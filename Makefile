# Compiler
CC = gcc
# Compiler flags
CFLAGS = -O0
# Linker flags
LFLAGS = -lm

# Directories
SRC_DIR = ./src
RAW_SOCKETS_DIR = ./src/raw_sockets
OBJ_DIR = ./obj

# Source files
SERVER_SRC_FILES := $(shell find $(SRC_DIR)/server -name '*.c')
CLIENT_SRC_FILES := $(shell find $(SRC_DIR)/client -name '*.c')
RAW_SOCKETS_SRC_FILES := $(shell find $(RAW_SOCKETS_DIR) -name '*.c')

# Object files
SERVER_OBJ_FILES := $(patsubst $(SRC_DIR)/server/%.c, $(OBJ_DIR)/server/%.o, $(SERVER_SRC_FILES))
CLIENT_OBJ_FILES := $(patsubst $(SRC_DIR)/client/%.c, $(OBJ_DIR)/client/%.o, $(CLIENT_SRC_FILES))
RAW_SOCKETS_OBJ_FILES := $(patsubst $(RAW_SOCKETS_DIR)/%.c, $(OBJ_DIR)/raw_sockets/%.o, $(RAW_SOCKETS_SRC_FILES))

# Executables
SERVER_EXECUTABLE = server
CLIENT_EXECUTABLE = client

# Default target
all: $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

# Server executable
$(SERVER_EXECUTABLE): $(SERVER_OBJ_FILES) $(RAW_SOCKETS_OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# Client executable
$(CLIENT_EXECUTABLE): $(CLIENT_OBJ_FILES) $(RAW_SOCKETS_OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# Server object files
$(OBJ_DIR)/server/%.o: $(SRC_DIR)/server/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Client object files
$(OBJ_DIR)/client/%.o: $(SRC_DIR)/client/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Raw sockets object files
$(OBJ_DIR)/raw_sockets/%.o: $(RAW_SOCKETS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR)
	rm -f $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

# Purge target
purge: clean
	@echo "Purging all..."
	rm -f $(DISTDIR) $(DISTDIR).tar

# Distribution target
dist: purge
	@echo "Creating distribution archive ($(DISTDIR).tar)..."
	@ln -s . $(DISTDIR)
	@tar -cvf $(DISTDIR).tar $(addprefix ./$(DISTDIR)/, $(DISTFILES))
	@rm -f $(DISTDIR)

# Phony targets
.PHONY: all clean purge dist
