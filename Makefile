CXX=g++
CXXFLAGS=-O2 -std=c++17 -Wall -Wextra -pthread
INCLUDES=-Ithird_party/rapidjson/include
LIBS=-lcurl

BIN_DIR=bin
SRC_DIR=src

all: dirs deps seq par

dirs:
	mkdir -p $(BIN_DIR)

deps:
	./scripts/setup_deps.sh

seq: $(SRC_DIR)/sequential.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)/bfs_seq $^ $(LIBS)

par: $(SRC_DIR)/parallel.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)/bfs_par $^ $(LIBS)

clean:
	rm -rf $(BIN_DIR)
