CC=gcc
FLAGS=-g -fPIC
FLAGS_TEST=-g

LIBS=-lpthread

SRC=wesock.c

OBJ=wesock.o

SLIB_NAME=libwe.a

all:
	$(CC) $(FLAGS) -c $(SRC) $(INCLUDES)
	ar -r $(SLIB_NAME) $(OBJ) 

test:
	$(CC) $(FLAGS_TEST) -o wesock.serv wesock_test_server.c -L. -lwe $(LIBS) 
	$(CC) $(FLAGS_TEST) -o wesock.clnt wesock_test_client.c -L. -lwe $(LIBS)

clean:
	rm -f $(SLIB_NAME)
	rm -f $(OBJ)
	rm -f wesock.serv
	rm -f wesock.clnt
