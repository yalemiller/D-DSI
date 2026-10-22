# Host build of the platform-independent code (source/core, source/gfx, source/ui)
# with the unit tests and the screenshot runner. Run from the repo root:
#   make -C tests          build and run the unit tests
#   make -C tests shots    render scripted screens to shots/*.png
CC ?= gcc
WARN := -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Werror
SAN := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
CFLAGS ?= -std=c11 -O1 -g $(WARN) $(SAN)
CPPFLAGS := -I../source -I../assets -I.

CORE := $(wildcard ../source/core/*.c)
GFX := $(wildcard ../source/gfx/*.c)
UI := $(wildcard ../source/ui/*.c)
APP := $(CORE) $(GFX) $(UI)
TESTS := $(wildcard test_*.c) main.c memfs.c

all: test

build/run_tests: $(APP) $(TESTS) $(wildcard *.h) $(wildcard ../source/*/*.h)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP) $(TESTS) -o $@

test: build/run_tests
	cd .. && tests/build/run_tests

build/shots: $(APP) shots.c memfs.c png.c $(wildcard *.h) $(wildcard ../source/*/*.h)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP) shots.c memfs.c png.c -o $@

shots: build/shots
	cd .. && tests/build/shots shots

clean:
	rm -rf build

.PHONY: all test shots clean
