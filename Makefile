CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET = db_engine
SRC = src/main.cpp
TEST_TARGET := build/tests
TEST_SOURCES := $(wildcard tests/test_*.cpp)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -rf $(TARGET) build

$(TEST_TARGET): $(TEST_SOURCES) tests/test_framework.hpp tests/fixtures.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Itests $(TEST_SOURCES) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

.PHONY: all clean test
