GCC := gcc
GCC_FLAGS := -std=gnu11 -Werror -Wall -Wno-unused-result

CFLAGS := -O2
LDFLAGS :=

CFLAGS_D := $(CFLAGS) -DDEBUG -DDEBUG_SDL
LDFLAGS_D := $(LDFLAGS)

CFLAGS_CPUD := $(CFLAGS) -DDEBUG
LDFLAGS_CPUD := $(LDFLAGS)

SRC_DIR := src

ifeq ($(OS),Windows_NT)
	PYTHON := python
	BIN_EXT := .exe
	LIBS := -lmingw32 -lSDL2main -lSDL2 
else
	PYTHON := python3
	BIN_EXT :=
	LIBS := -lSDL2
endif

BIN_DIR := bin
BIN_NAME := dndltr$(BIN_EXT)
BIN_FULLNAME := $(BIN_DIR)/$(BIN_NAME)
BIN_NAME_D := dndltr_d$(BIN_EXT)
BIN_FULLNAME_D := $(BIN_DIR)/$(BIN_NAME_D)
BIN_NAME_CPUD := dndltr_cpud$(BIN_EXT)
BIN_FULLNAME_CPUD := $(BIN_DIR)/$(BIN_NAME_CPUD)

TESTS_DIR := tests

SRCS := $(SRC_DIR)/main.c \
        $(SRC_DIR)/error.c \
        $(SRC_DIR)/pars.c \
        $(SRC_DIR)/sdl_manager.c \
        $(SRC_DIR)/nes_ppu.c \
        $(SRC_DIR)/nes_apu.c \
        $(SRC_DIR)/nes_mappers.c \
        $(SRC_DIR)/nes_cart.c \
        $(SRC_DIR)/nes.c \
        $(SRC_DIR)/nes_input.c \
        $(SRC_DIR)/core.c

ifeq ($(OS),Windows_NT)
	SEP := \\
	RM := del /f
else
	SEP := /
	RM := rm -rf
endif

default: $(BIN_DIR) $(BIN_FULLNAME)

debug: $(BIN_DIR) $(BIN_FULLNAME_D)

cpudebug: $(BIN_DIR) $(BIN_FULLNAME_CPUD)

$(BIN_FULLNAME): $(SRCS)
	$(GCC) $^ $(GCC_FLAGS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

$(BIN_FULLNAME_D): $(SRCS)
	$(GCC) $^ $(GCC_FLAGS) $(CFLAGS_D) $(LDFLAGS_D) $(LIBS) -o $@

$(BIN_FULLNAME_CPUD): $(SRCS)
	$(GCC) $^ $(GCC_FLAGS) $(CFLAGS_CPUD) $(LDFLAGS_CPUD) $(LIBS) -o $@

test: debug
	@$(PYTHON) $(TESTS_DIR)/run_tests.py

start: default
	$(BIN_FULLNAME)

$(BIN_DIR):
	-mkdir $@

.PHONY: clean test start
clean:
	-@$(RM) $(BIN_DIR)$(SEP)$(BIN_NAME)
	-@$(RM) $(BIN_DIR)$(SEP)$(BIN_NAME_D)


