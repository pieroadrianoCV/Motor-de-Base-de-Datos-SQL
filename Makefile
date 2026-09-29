CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET = db_engine
SRC = src/main.cpp
TEST_TARGET := build/tests
TEST_SOURCES := $(wildcard tests/test_*.cpp)
BENCHMARK_TARGET := build/benchmark

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -rf $(TARGET) build

$(TEST_TARGET): $(TEST_SOURCES) tests/test_framework.hpp tests/fixtures.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -Itests $(TEST_SOURCES) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BENCHMARK_TARGET): src/benchmark_main.cpp src/benchmark/BulkLoader.hpp src/benchmark/ScanBenchmark.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc src/benchmark_main.cpp -o $@

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET) $(ARGS)

.PHONY: all clean test benchmark
