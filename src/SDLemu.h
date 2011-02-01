/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: SDLemu.h                                                         *
 * Author: Mark Schmelzenbach                                             *
 * Started: 01/26/06                                                      *
 =========================================================================*/

#include "SDL.h"

#define MainContext 0
#define UpdContext 1
#define DialogContext 2
#define BackContext 3

int SDLNoUpdate();
int SDLUpdate();

int SDLsetPalette(int idx, int r, int g, int b);
int SDLPlot(int x, int y, int clrIdx);
int SDLplotchr(int sx, int sy, int chr, int typ, unsigned char *fnt);
int SDLmap_plotchr(int sx, int sy, int chr, int typ, unsigned char *fnt);
int SDLstring(int x, int y, char *txt);
int SDLBox(int x1, int y1, int x2, int y2, int clr);
int SDLHollowBox(int x1, int y1, int x2, int y2, int clr);
int SDLXORBox(int x1, int y1, int x2, int y2);
int SDLXORHollowBox(int x1, int y1, int x2, int y2);
int SDLLine(int x1, int y1, int x2, int y2, int clr);
int SDLHLine(int x1, int x2, int y, int clr);
int SDLVLine(int x1, int y1, int y2, int clr);

int SDLClear(int clr);
int SDLSetContext(int id);
int SDLClip(int hidden);

int SDLgetch(int click);
int SDLrelease();
int SDLTranslateClick(int x);

int SDLCreateCharsContext(int w, int h);

int SDLContextBlt(int dst, int dx, int dy, int src, int sx1, int sy1, int sx2, int sy2);
int SDLCharBlt(int dst, int dx, int dy, int idx);

int unpack(unsigned char *look);

int initSDL(int zoom, int fullScreen);
int toggleFullScreen();
int shutdownSDL();
int SDLRebuildChar(int idx, int w, int h);

extern SDL_Surface *charTable[512];
