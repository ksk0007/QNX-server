# ============================================================
# QNX Authoritative Game Server Makefile
# ============================================================

TARGET = QNX_GAME_SERVER

PLATFORM ?= aarch64le
BUILD_PROFILE ?= debug

CC = clang
CXX = clang++

CXXFLAGS = -Wall -Wextra -std=c++11

LDFLAGS = -lsocket -lm

SRC_DIR = src
BUILD_DIR = build/$(PLATFORM)-$(BUILD_PROFILE)

SOURCES = \
        $(SRC_DIR)/main.cpp \
        $(SRC_DIR)/player.cpp \
        $(SRC_DIR)/network.cpp \
        $(SRC_DIR)/game_world.cpp \
        $(SRC_DIR)/world_collision.cpp

OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

rebuild: clean all

.PHONY: all clean rebuild
