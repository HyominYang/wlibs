CC=g++
SRC=main.cpp \
	aria.cpp
INC=-I../sock
LIB=-lpthread -L../libs -lwe
DEF=

all:
	$(CC) -o mw  $(SRC) $(INC) $(LIB) $(DEF)

clean:
	rm -f mw
