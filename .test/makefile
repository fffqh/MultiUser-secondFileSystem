.PHONY: all clean

CC = g++
CFLAG = -c -Wall
SOURCE=$(wildcard *.cpp)
EXE=$(patsubst %.cpp,%,$(SOURCE))

all:$(EXE)

$(EXE) :%:%.o 
	$(CC) -o $@  $^  -lpthread

%.o:%.cpp 
	$(CC) $(CFLAG) -o $@ $^ 

clean:
	rm -f $(EXE) *.o