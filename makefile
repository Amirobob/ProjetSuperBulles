CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC_DIR = src
INC_DIR = src/headers
DIST_DIR = dist

SRC = $(SRC_DIR)/main.c \
      $(SRC_DIR)/affichage.c \
      $(SRC_DIR)/fichiers.c \
	  $(SRC_DIR)/interface.c \
	  $(SRC_DIR)/logique.c \

TARGET = $(DIST_DIR)/game

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(DIST_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $^ -o $@ -lalleg44

clean:
	rm -rf $(DIST_DIR)

run: all
	./$(TARGET)
