CC     = gcc
CFLAGS = -Wall -Wextra -g
INC    = -Icore/inc

SRC    = core/src/city_manager.c \
         core/src/district.c     \
         core/src/report.c       \
         core/src/utils.c

OBJ    = $(patsubst core/src/%.c, build/%.o, $(SRC))
TARGET = city_manager

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

build/%.o: core/src/%.c | build
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

.PHONY: all clean build