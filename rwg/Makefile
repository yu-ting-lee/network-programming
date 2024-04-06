CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17

SRC1 = $(wildcard ./np-simple/*.cpp)
SRC2 = $(wildcard ./np-single-proc/*.cpp)
SRC3 = $(wildcard ./np-multi-proc/*.cpp)

EXE = np_multi_proc np_single_proc np_simple

LIB = -lpthread


.PHONY: all np_simple np_single_proc np_multi_proc clean

all: np_simple np_single_proc np_multi_proc

np_simple:
	$(CXX) $(CXXFLAGS) $(SRC1) -o np_simple

np_single_proc:
	$(CXX) $(CXXFLAGS) $(SRC2) -o np_single_proc

np_multi_proc:
	$(CXX) $(CXXFLAGS) $(SRC3) -o np_multi_proc $(LIB)

clean:
	@rm -f $(EXE)