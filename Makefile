CC = gcc
CFLAGS = -Wall -Wextra -g
INC = -Icore/inc

# city_manager sources
CM_SRC = core/src/city_manager.c \
    core/src/district.c \
    core/src/report.c \
	core/src/utils.c
CM_OBJ = $(patsubst core/src/%.c, build/%.o, $(CM_SRC))

# monitor_reports sources
MON_SRC = core/src/monitor_reports.c \
    core/src/utils.c
MON_OBJ = $(patsubst core/src/%.c, build/%.o, $(MON_SRC))

# city_hub sources
HUB_SRC = core/src/city_hub.c \
	core/src/utils.c
HUB_OBJ = $(patsubst core/src/%.c, build/%.o, $(HUB_SRC))

all: city_manager monitor_reports city_hub

city_manager: $(CM_OBJ)
	$(CC) $(CFLAGS) -o city_manager $(CM_OBJ)

monitor_reports: $(MON_OBJ)
	$(CC) $(CFLAGS) -o monitor_reports $(MON_OBJ)

city_hub: $(HUB_OBJ)
	$(CC) $(CFLAGS) -o city_hub $(HUB_OBJ)

build/%.o: core/src/%.c | build
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build city_manager monitor_reports city_hub

.PHONY: all clean build