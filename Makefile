CXX=g++
CXXFLAGS=-Iinclude -Wall
SRC_FILES=$(wildcard src/*.cxx)
HEADER_FILES=$(wildcard include/*.cxx)
OBJ_FILES=$(SRC_FILES:.cxx=.o)
TARGET=program

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) -o $@ $^

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET)

.PHONY: all clean
