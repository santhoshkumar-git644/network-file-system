CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude -g

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

CLIENT_SRC = $(wildcard $(SRC_DIR)/client/*.c) $(wildcard $(SRC_DIR)/utils/*.c)
NM_SRC = $(wildcard $(SRC_DIR)/naming_server/*.c) $(wildcard $(SRC_DIR)/utils/*.c)
SS_SRC = $(wildcard $(SRC_DIR)/storage_server/*.c) $(wildcard $(SRC_DIR)/utils/*.c)

CLIENT_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SRC))
NM_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(NM_SRC))
SS_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SS_SRC))

all: directories client naming_server storage_server

directories:
	@mkdir -p $(OBJ_DIR)/client $(OBJ_DIR)/naming_server $(OBJ_DIR)/storage_server $(OBJ_DIR)/utils $(BIN_DIR)

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/client $^

naming_server: $(NM_OBJ)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/naming_server $^

storage_server: $(SS_OBJ)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/storage_server $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean directories
