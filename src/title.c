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

int credits(int x, int y)
{
	SDLstring(44+x,59+y,"Atari Font Editor");
	SDLstring(250+x,50+y,"-Reborn");
	int center;
	SDLBox(8,CONFIG.screenHeight-12,CONFIG.screenWidth-8,CONFIG.screenHeight-5,1);
	SDLBox(8,CONFIG.screenHeight-24,CONFIG.screenWidth-8,CONFIG.screenHeight-17,1);
	char * title="v0.8.53 by Jakub Husak with help of STC.";
	center=(CONFIG.screenWidth-strlen(title)*8)/2;
	SDLstring(center,CONFIG.screenHeight-24,title);
	title="v0.8 Programmed by Mark Schmelzenbach";
	center=(CONFIG.screenWidth-strlen(title)*8)/2;
	SDLstring(center,CONFIG.screenHeight-12,title);
	return 1;
}

int title_header()
{
	titlepal();
	int cx=CONFIG.screenWidth/2-180;
	int cy=20;
	unpack(titles,cx,cy);
	SDLstring(44+cx,59+cy,"Atari Font Editor");
	SDLstring(250+cx,50+cy,"-Reborn");

	return 1;
}


int title()
{
	titlepal();
	int cx=CONFIG.screenWidth/2-180;
	int cy=CONFIG.screenHeight/2-60;
	SDLClear(0);
	unpack(titles,cx,cy);
	credits(cx,cy);
	SDLgetch(1);
	return 1;
}
