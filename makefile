CC=g++

OS_NAME := linux

ifeq ($(OS),Windows_NT)
  OS_NAME := win32
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    OS_NAME := darwin
  endif
endif

CXXFLAGS=-I. -I${JAVA_HOME}/include -I${JAVA_HOME}/include/$(OS_NAME) -ldl -std=c++11

SRC=jnipp.o main.o
VPATH=tests

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

test: $(SRC)
	$(CC) -o test $(SRC) $(CXXFLAGS)

