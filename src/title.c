/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: title.c                                                          *
 * Author: Mark Schmelzenbach                                             *
 * Started: 08/04/97                                                      *
 * Long overdue port to SDL 1/26/2006                                     *
 =========================================================================*/
#include "SDLemu.h"

#include "raw.h"
#include "envision.h"

void titlepal()
{
	unsigned char r,g,b;
	int x;

	for(x=0;x<256;x++) {
		r=(tpal[x*3]);
		g=(tpal[x*3+1]);
		b=(tpal[x*3+2]);
		SDLsetPalette(x,r,g,b);
	}
}

int credits()
{
	SDLBox(8,191,319-8,200,1);
	SDLstring(44,126,"Atari Font Editor");
	SDLstring(250,120,"-Reborn");
	SDLstring(12,181,"v0.8.13 by Jakub Husak and STC.");
	SDLstring(12,191,"v0.8 Programmed by Mark Schmelzenbach");
	return 1;
}

int title()
{
	SDLNoUpdate();
	titlepal();
	unpack(titles);
	credits();
	SDLUpdate();
	SDLgetch(1);
	return 1;
}
