CXX=g++
CXXFLAGS=-std=c++23 -Wall -Wextra -g
BIN=sleep_server
CNT=lima nerdctl
IMG=sleep-server

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) -o $(BIN) $^

%.o: %.c
	$(CXX) $@ -c $<

clean:
	rm -f *.o $(BIN)

image:
	$(CNT) build -t $(IMG) .

run: image
	$(CNT) run -it --rm $(IMG) ./$(BIN) $(_)
