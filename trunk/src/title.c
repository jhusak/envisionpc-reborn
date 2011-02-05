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

int credits(int x, int y)
{
	currentView->dc=2;
	SDLstring(44+x,127+y,"Atari Font Editor");
	SDLstring(250+x,120+y,"-Reborn");
	int center=(CONFIG.screenWidth-280)/2;
	SDLBox(8,CONFIG.screenHeight-12,CONFIG.screenWidth-8,CONFIG.screenHeight-5,1);
	SDLBox(8,CONFIG.screenHeight-24,CONFIG.screenWidth-8,CONFIG.screenHeight-17,1);
	SDLstring(center,CONFIG.screenHeight-24,"v0.8.17 by Jakub Husak and STC.");
	SDLstring(center,CONFIG.screenHeight-12,"v0.8 Programmed by Mark Schmelzenbach");
	return 1;
}

int title()
{
	SDLNoUpdate();
	titlepal();
	int cx=CONFIG.screenWidth/2-180;
	int cy=CONFIG.screenHeight/2-130;
	unpack(titles,cx,cy);
	credits(cx,cy);
	SDLUpdate();
	SDLgetch(1);
	return 1;
}
