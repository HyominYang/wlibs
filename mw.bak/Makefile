CC=g++
INC=-I../sock -I../confr
LIB=-lpthread -L../libs -lwe -lwc
MK_LIC=-DMAKE_LIC

SRC_CLI=main.cpp \
	aria.cpp \
	network.cpp \
	util.cpp

SRC_SERV=serv.cpp \
		 aria.cpp \
		 network_serv.cpp \
		 util.cpp

all: client server

client:
	$(CC) -o mw  $(SRC_CLI) $(INC) $(LIB)

server:
	$(CC) -o serv  $(SRC_SERV) $(INC) $(LIB)

lic:
	$(CC) -o lic_gen $(SRC) $(INC) $(LIB) $(MK_LIC)

clean:
	rm -f mw serv lic_gen
