CXX := clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
SRC := src/compressor.cpp
BIN := bin/compressor

.PHONY: all clean run

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: all
	python3 app.py

clean:
	rm -f $(BIN)
