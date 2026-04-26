CC = gcc
CFLAGS = -Wall -Wextra -g
INC = -Icore/inc

SRC = core/src/city_manager.c
OBJ = $(SRC:.c=.o)
TARGET = city_manager

all: $(TARGET)

# Link step
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile step: each .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean