CC=gcc
FLAGS=-Wall -g -fPIC
LIBS= -I../sock -L../libs -lpthread -lwe
SRC_CLIENT=main_client.c \
	../encrypt/aria.c
SRC_SERV=main_serv.c \
	../encrypt/aria.c

TARGET_CLIENT=wclient
TARGET_SERV=wserver

all: client server

client:
	$(CC) $(FLAGS) -o $(TARGET_CLIENT) $(SRC_CLIENT) $(INCLUDES) $(LIBS)

server:
	$(CC) $(FLAGS) -o $(TARGET_SERV) $(SRC_SERV) $(INCLUDES) $(LIBS)
