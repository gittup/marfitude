CC = gcc
#CFLAGS = `sdl-config --cflags` -DLOG -DDEBUG_MEM -fomit-frame-pointer -c -Wall -O3
CFLAGS = `sdl-config --cflags` -DLOG -fomit-frame-pointer -c -Wall -O3
#CFLAGS = `sdl-config --cflags` -fomit-frame-pointer -c -Wall -O3
LIBS = `sdl-config --libs` -lm -lSDL_image -lSDL_mixer -lGL -lGLU
UTILS = ../util/fatalerror.o ../util/sdlfatalerror.o ../util/savetga.o ../util/token.o ../util/textprogress.o ../util/strfunc.o ../util/memtest.o
OBJS = audio.o cfg.o event.o fps.o glfunc.o joy.o log.o main.o menu.o module.o munitrk.o phys.o scene.o sounds.o textures.o timer.o wam.o $(UTILS)
TARGET = gmae
gmae: $(OBJS)
	gcc $(OBJS) -o $(TARGET) $(LIBS)

audio.o: audio.c cfg.h ../util/sdlfatalerror.h
	$(CC) $(CFLAGS) audio.c
cfg.o: cfg.c log.h ../util/memtest.h ../util/fatalerror.h ../util/token.h ../util/strfunc.h
	$(CC) $(CFLAGS) cfg.c
event.o: event.c event.h main.h menu.h joy.h cfg.h log.h ../util/memtest.h ../util/strfunc.h
	$(CC) $(CFLAGS) event.c
fps.o: fps.c glfunc.h timer.h
	$(CC) $(CFLAGS) fps.c
glfunc.o: glfunc.c glfunc.h fps.h cfg.h textures.h log.h ../util/memtest.h ../util/sdlfatalerror.h
	$(CC) $(CFLAGS) glfunc.c
joy.o: joy.c cfg.h log.h ../util/memtest.h
	$(CC) $(CFLAGS) joy.c
log.o: log.c
	$(CC) $(CFLAGS) log.c
main.o: main.c main.h event.h joy.h audio.h glfunc.h menu.h cfg.h scene.h log.h textures.h timer.h wam.h ../util/memtest.h
	$(CC) $(CFLAGS) main.c
menu.o: menu.c menu.h main.h event.h glfunc.h sounds.h textures.h scene.h cfg.h log.h ../util/memtest.h ../util/fatalerror.h
	$(CC) $(CFLAGS) menu.c
module.o: module.c module.h cfg.h log.h wam.h ../util/memtest.h ../util/sdlfatalerror.h
	$(CC) $(CFLAGS) module.c
scene.o: scene.c scene.h cfg.h event.h glfunc.h log.h main.h menu.h module.h phys.h textures.h wam.h ../util/fatalerror.h ../util/memtest.h ../util/sdlfatalerror.h
	$(CC) $(CFLAGS) scene.c
sounds.h: sounds/
	./gensounds.pl
sounds.o: sounds.c sounds.h log.h ../util/memtest.h ../util/textprogress.h ../util/fatalerror.h ../util/sdlfatalerror.h
	$(CC) $(CFLAGS) sounds.c
phys.o: phys.c phys.h log.h
	$(CC) $(CFLAGS) phys.c
textures.h: images/
	./gentextures.pl
textures.o: textures.c textures.h glfunc.h log.h ../util/memtest.h ../util/textprogress.h ../util/fatalerror.h
	$(CC) $(CFLAGS) textures.c
timer.o: timer.c
	$(CC) $(CFLAGS) timer.c
wam.o: wam.c wam.h cfg.h log.h module.h ../util/fatalerror.h ../util/memtest.h ../util/textprogress.h
	@echo "Removing old WAM files";
	rm -f wam/*.wam
	$(CC) $(CFLAGS) wam.c

clean:
	rm -f *.o
	rm -f $(TARGET)
	rm -f $(UTILS)
	touch images
	touch sounds
