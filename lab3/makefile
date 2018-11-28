COMPILER=g++ -std=c++0x
EXT=cpp
CFLAGS=-Wall -Werror
EXECUTABLE=lab3

SOURCES=$(shell find . -name "*.$(EXT)")

all: $(SOURCES)
	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(EXECUTABLE) -lpthread
clean:
	rm -f *.o $(EXECUTABLE)
