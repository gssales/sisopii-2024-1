CXX=g++
CXXFLAGS=-std=c++23 -Wall -Wextra -I. -g
BIN=sleep_server
CNT=docker
IMG=sleep-server
NAME=host1

SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) -o $(BIN) $^

clean:
	rm -f **/*.o $(BIN)

image:
	$(CNT) build -t $(IMG) .

run: image
	$(CNT) run -it --rm --name $(NAME) -h $(NAME) $(IMG) ./$(BIN) $(_)
