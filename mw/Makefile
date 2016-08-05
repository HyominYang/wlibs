CC=g++
INC=-I../sock
LIB=-lpthread -L../libs -lwe
MK_LIC=-DMAKE_LIC

SRC=main.cpp \
	aria.cpp \
	network.cpp

all:
	$(CC) -o mw  $(SRC) $(INC) $(LIB)

lic:
	$(CC) -o lic_gen $(SRC) $(INC) $(LIB) $(MK_LIC)

clean:
	rm -f mw
