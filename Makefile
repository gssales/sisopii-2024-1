CXX=g++
CXXFLAGS=-std=c++23 -Wall -Wextra -g
BIN=hello_world

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) -o $(BIN) $^

%.o: %.c
	$(CXX) $@ -c $<

clean:
	rm -f *.o $(BIN)

docker:
	docker build -t $(BIN) .

run:
	docker run --rm $(BIN)
