CXX = g++
CXXFLAGS = -O3 -Wall

SRC1 = http.cpp utils.cpp
SRC2 = console.cpp utils.cpp

EXE = http console.cgi

LIB = -lpthread


.PHONY: all http console clean

all: http console

http:
	$(CXX) $(CXXFLAGS) $(SRC1) -o http $(LIB)

console:
	$(CXX) $(CXXFLAGS) $(SRC2) -o console.cgi $(LIB)

clean:
	@rm -f $(EXE)