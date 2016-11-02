CC=g++
CXXFLAGS=-I. -I/usr/lib/jvm/default-java/include -ldl -std=c++11
SRC=internal.o jnipp.o main.o
VPATH=tests

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

test: $(SRC)
	$(CC) -o test $(SRC) $(CXXFLAGS)

