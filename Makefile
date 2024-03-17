CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-pthread
OBJ=projetoFinal.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

projetoFinal: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) projetoFinal
