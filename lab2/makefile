CC=gcc # Current compiler
CFLAGS=-Wall -Werror # Source files build flags
LIBS=-lpthread -lrt # Libs to be connected with
HOST_SOURCE=host.c # Host source file name

# Get executables names
CONN_SOURCES=$(shell find . -name "conn_*.c")
EXECUTABLES=$(CONN_SOURCES:./conn_%.c=host_%)

all: $(EXECUTABLES)
	
host_%: conn_%.o $(HOST_SOURCE:.c=.o)
	$(CC) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ 

.PRECIOUS: %.o # Do not delete intermediate object files

clean:
	rm -rf *.o $(EXECUTABLES)
