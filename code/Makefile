CFLAGS = -Wall -g -O0

default: main.o fileio.o queue.o debug.o controller.o
	gcc -g $(CFLAGS) -o mem main.o fileio.o queue.o debug.o controller.o

main.o: main.c
	gcc -c $(CFLAGS) main.c

fileio.o: fileio.c fileio.h
	gcc -c $(CFLAGS) fileio.c fileio.h

queue.o: queue.c queue.h
	gcc -c $(CFLAGS)  queue.c queue.h

debug.o: debug.c debug.h
	gcc -c $(CFLAGS) debug.c debug.h

controller.o: controller.c controller.h
	gcc -c $(CFLAGS) controller.c controller.h

clean:
	rm -f *.o *.gch mem clean
