all: timeapp

timeapp: timeapp.o
	$(CC) -o timeapp timeapp.o $(LDFLAGS)

timeapp.o: timeapp.c
	$(CC) $(CFLAGS) -c timeapp.c

clean:
	rm -f *.o
