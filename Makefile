CC = gcc
CFLAGS = -I$(IDIR)

make:
	$(CC) agatetepe.c -o agatetepe -lsocket -lpthread

makewin:
	$(CC) agatetepe.c -o agatetepe -lws2_32 -lpthread

.PHONY: clean

clean:
	rm -f *.o