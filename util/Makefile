CC = gcc
CFLAGS = `sdl-config --cflags`
LIBS = `sdl-config --libs` -lm -lSDL -lpthread
OBJS = ppm.o fatalerror.o wav.o font.o savetga.o
lib: $(OBJS)
	ld -r -o libmarf.a $(OBJS)
clean:
	rm -f *.o
#ppm: ppm.h
#	gcc $(CFLAGS) -c ppm.c
#fatalerror: fatalerror.h
#	gcc $(CFLAGS) -c fatalerror.c
#wav: wav.h
#	gcc $(CFLAGS) -c wav.c
#font: font.h
#	gcc $(CFLAGS) -c font.c
