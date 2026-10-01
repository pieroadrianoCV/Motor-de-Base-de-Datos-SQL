CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET = db_engine
SRC = src/main.cpp
TEST_TARGET := build/tests
TEST_SOURCES := $(wildcard tests/test_*.cpp)
BENCHMARK_TARGET := build/benchmark
DATA_FILE ?= data/database.bin

INDEX_HEADERS := $(wildcard src/index/*.hpp)
STORAGE_HEADERS := $(wildcard src/storage/*.hpp)
BENCHMARK_HEADERS := $(wildcard src/benchmark/*.hpp)
DEMO_HEADERS := $(wildcard src/demo/*.hpp)

all: $(TARGET)

$(TARGET): $(SRC) $(INDEX_HEADERS) $(STORAGE_HEADERS) $(BENCHMARK_HEADERS) $(DEMO_HEADERS)
	$(CXX) $(CXXFLAGS) -Isrc -o $(TARGET) $(SRC)

clean:
	rm -rf $(TARGET) build

$(TEST_TARGET): $(TEST_SOURCES) tests/test_framework.hpp $(INDEX_HEADERS) $(STORAGE_HEADERS) $(BENCHMARK_HEADERS) $(DEMO_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -Itests $(TEST_SOURCES) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BENCHMARK_TARGET): src/benchmark_main.cpp $(BENCHMARK_HEADERS) $(INDEX_HEADERS) $(STORAGE_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc src/benchmark_main.cpp -o $@

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET) $(ARGS)

run: $(TARGET)
	./$(TARGET) $(DATA_FILE)

clean-data:
	rm -f $(DATA_FILE) $(DATA_FILE).tmp

.PHONY: all clean clean-data test benchmark run
