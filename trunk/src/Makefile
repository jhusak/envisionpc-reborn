CFLAGS  = -O3 -I/usr/include/SDL -Wall -I/Library/Frameworks/SDL.framework/Versions/A/Headers/
L       = -O3 -lSDL
L       = -O3 -L /Library/Frameworks/SDL.framework/Versions/A/SDL
CC      = gcc

.c.s:
	$(CC) $(CFLAGS) -c $<

all:    prog

OBJS    = \
	dialogs.o envision.o fileio.o map.o title.o SDLemu.o util.o

prog : $(OBJS)
	$(CC) -o envision $(OBJS) $(L)

clean:
	rm *.o *~ envision

dialogs.o: SDLemu.h envision.h
envision.o: SDLemu.h colours.h font.h envision.h
fileio.o: envision.h
map.o: SDLemu.h envision.h
SDLemu.o: SDLemu.h envision.h icon.h
title.o: SDLemu.h raw.h envision.h
util.o: envision.h
