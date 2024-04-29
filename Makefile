CXX=g++
CXXFLAGS=-std=c++23 -Wall -Wextra -I. -g
BIN=sleep_server
CNT=nerdctl
IMG=sleep-server

SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(BIN) $^

clean:
	rm -f **/*.o $(BIN)

image:
	$(CNT) build -t $(IMG) .

run: image
	$(CNT) run -it --rm $(IMG) ./$(BIN) $(_)
