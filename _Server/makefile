CC=gcc
CFLAGS=-I.
LIBS= -lcurses -pthread -lrt
DEPS = server.h
OBJ = main.o server.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)