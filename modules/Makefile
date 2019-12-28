# compiler flags
CFLAGS= -O2 -Wall

all: consoleif.o datalogger.o datastreamer.o remote.o

consoleif.o: consoleif.c modules.h
	gcc $(CFLAGS) -c consoleif.c -o consoleif.o -lncurses

datalogger.o: datalogger.c modules.h
	gcc $(CFLAGS) -c datalogger.c -o datalogger.o

datastreamer.o: datastreamer.c modules.h
	gcc $(CFLAGS) -c datastreamer.c -o datastreamer.o

remote.o: remote.c modules.h
	gcc $(CFLAGS) -c remote.c -o remote.o

clean:
	rm -f *.o

stats:
	wc -l *.c *.h */*.c */*.h
