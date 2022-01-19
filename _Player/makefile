CC=gcc
CFLAGS=-I.
LIBS= -lcurses -pthread -lrt
DEPS = player.h
OBJ = main.o player.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)