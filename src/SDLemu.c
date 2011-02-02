/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: SDLemu.c - a wrapper around ancient GRX2.0 functionality         *
 * Author: Mark Schmelzenbach                                             *
 * Started: 01/26/2006                                                    *
 =========================================================================*/
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDLemu.h"

#include "envision.h"
#include "icon.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const Uint32 rmask = 0xff000000;
const Uint32 gmask = 0x00ff0000;
const Uint32 bmask = 0x0000ff00;
const Uint32 amask = 0x000000ff;
#else
const Uint32 rmask = 0x000000ff;
const Uint32 gmask = 0x0000ff00;
const Uint32 bmask = 0x00ff0000;
const Uint32 amask = 0xff000000;
#endif

typedef struct screen {
	int zoom, fullScreen, update;
	SDL_Surface *current;
	SDL_Surface *surfaces[5];
	Uint32 *clut;
	Uint32 *rgb;
} screen;

static screen *mainScreen;
static SDL_Surface *iconW;

/*===========================================================================
 * freeScreen
 * free SDL resources
 * param kill: the screen to destroy
 * returns: nothing useful
 *==========================================================================*/
int freeScreen(screen *kill)
{
	int i;
	for(i=1;i<4;i++)
		SDL_FreeSurface(kill->surfaces[i]);
	free(kill->clut);
	free(kill->rgb);
	free(kill);
	return 1;
}

/*===========================================================================
 * getScreen
 * allocate a new SDL screen resource
 * param zoom: zoom level
 * param fullScreen: flag indicating full screen should be used
 * returns: the new screen resource
 *==========================================================================*/
screen *getScreen(int zoom, int fullScreen)
{
	int i;
	screen *ret=(screen *)malloc(sizeof(screen));

	ret->current=NULL;
	ret->zoom=zoom;
	ret->update=1;
	ret->fullScreen=!fullScreen;
	ret->clut=(Uint32 *)malloc(sizeof(Uint32)*256);
	ret->rgb=(Uint32 *)malloc(sizeof(Uint32)*256);
	memset(ret->clut,0,256);
	memset(ret->rgb,0,256);
	for(i=0;i<4;i++)
		ret->surfaces[i]=NULL;
	return ret;
}

/*===========================================================================
 * SDLNoUpdate
 * turn off screen updates
 * returns: nothing useful
 *==========================================================================*/
int SDLNoUpdate()
{
	mainScreen->update=0;
	return 1;
}

/*===========================================================================
 * SDLUpdate
 * turn on screen updates, and refresh entire screen
 * returns: nothing useful
 *==========================================================================*/
int SDLUpdate()
{
	mainScreen->update=1;
	SDL_UpdateRect(mainScreen->surfaces[0],0,0,0,0);
	return 1;
}

/*===========================================================================
 * SDLClip
 * set SDL clipping window
 * param hidden: flag indicating mode (0=fill screen; 1=map + menu; 2=map)
 * returns: nothing useful
 *==========================================================================*/
int SDLClip(int hidden) {
	SDL_Rect r;

	if (hidden<=0)
		SDL_SetClipRect(mainScreen->current,NULL);
	else {
		r.x=0; r.y=24*mainScreen->zoom;
		if (hidden==1) {
			r.h=200*mainScreen->zoom; r.w=248*mainScreen->zoom;
		} else {
			r.h=200*mainScreen->zoom; r.w=320*mainScreen->zoom;
		}
		SDL_SetClipRect(mainScreen->current,&r);
	}
	return 1;
}

/*===========================================================================
 * SDLRebuildChar
 * rebuild a character
 * param idx: character to build
 * param w: width in pixels
 * param h: height in pixels
 * returns: nothing useful
 *==========================================================================*/
int SDLRebuildChar(int idx, int w, int h) {
	SDL_Surface *s;
	if (charTable[idx]) {
		SDL_FreeSurface(charTable[idx]);
		charTable[idx]=NULL;
	}
	if ((!w)&&(!h))
		return 1;

	s=SDL_CreateRGBSurface(SDL_SWSURFACE,w*mainScreen->zoom,h*mainScreen->zoom,32,rmask, gmask, bmask, amask);  // chars
	charTable[idx]=SDL_DisplayFormat(s);
	SDL_FreeSurface(s);
	mainScreen->current=charTable[idx];

	return 1;
}

/*===========================================================================
 * SDLString
 * print text to screen
 * param x: x-position of upper-left corner of text
 * param y: y-position upper-left corner of text
 * param str: the text to print
 * returns: nothing useful
 *==========================================================================*/
int SDLstring(int x, int y, char *str)
{
	int sx, c, i;

	if (!str) return 0;

	sx=x;
	for(i=0;i<strlen(str);i++) {
		c=*(str+i);
		if ((c<97)||(c>123)) c=c-32;
		if (c) SDLplotchr(sx,y,c,2,dfont);
		sx+=8;
	}
	return 1;
}

/*===========================================================================
 * SDLsetPalette
 * store the color palette registers
 * param idx: the index to update
 * param r: red channel (0-255);
 * param g: green channel (0-255);
 * param b: blue channel (0-255);
 * returns: nothing useful
 *==========================================================================*/
int SDLsetPalette(int idx, int r, int g, int b)
{
	if ((idx>=0)&&(idx<256)) {
		mainScreen->clut[idx]=SDL_MapRGB(mainScreen->surfaces[0]->format,r,g,b);
		mainScreen->rgb[idx]=((r&0xff)<<16)|((g&0xff)<<8)|(b&0xff);
		return 1;
	}
	return 0;
}

/*===========================================================================
 * SDLPlot
 * plot a (zoomed) pixel
 * param x: x-position
 * param y: y-position
 * param clrIdx: color
 * returns: nothing useful
 *==========================================================================*/
int SDLPlot(int x, int y, int clrIdx)
{
	Uint32 clr=mainScreen->clut[clrIdx];

	if (mainScreen->zoom>1) {
		SDL_Rect rect;
		rect.x=x*mainScreen->zoom;
		rect.y=y*mainScreen->zoom;
		rect.w=rect.h=mainScreen->zoom;
		SDL_FillRect(mainScreen->current,&rect,clr);
	} else {
		int bpp = mainScreen->current->format->BytesPerPixel;
		Uint8 *p=(Uint8 *)mainScreen->current->pixels+y*mainScreen->current->pitch+x*bpp;
		switch(bpp) {
			case 2:
				*(Uint16 *)p = clr;
				break;
			case 3:
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
					p[0] = (clr>>16)&0xff;
					p[1] = (clr>>8)&0xff;
					p[2] = clr&0xff;
				} else {
					p[0]=clr&0xff;
					p[1]=(clr>>8)&0xff;
					p[2]=(clr>>16)&0xff;
				}
				break;
			case 4:
				*(Uint32 *)p=clr;
				break;
			default:
				return 0;
		}
	}
	return 1;
}

/*===========================================================================
 * SDLCharEngine
 * plot a character in a given ANTIC mode on screen
 * param sx: x-position
 * param sy: y-position
 * param chr: character to plot
 * param typ: ANTIC mode (+mode 1, which is Gr.0 with no background)
 * param clr: color channel (may not be used)
 * param dat: pointer to character data
 * returns: nothing useful
 *==========================================================================*/
int SDLCharEngine(int sx, int sy, int chr, int typ, unsigned char clr, unsigned char *dat)
{
	unsigned char c,l;
	int x,y,cw,ch;

	if ((mainScreen->zoom==1)&&(SDL_MUSTLOCK(mainScreen->current))) {
		SDL_LockSurface(mainScreen->current);
	}

	switch (typ) {
		case 1: { /* Gr. 0, no background */
				for(y=0;y<8;y++) {
					c=*(dat+y);
					if (chr>=128) c=255-c;
					for(x=0;x<8;x++) {
						if (c&128) SDLPlot(sx+x,sy+y,clr);
						c=c<<1;
					}
				}
				cw=ch=8;
				break;
			}
		case 2: { /* Gr. 0 */
				for(y=0;y<8;y++) {
					c=*(dat+y);
					if (chr>=128) c=255-c;
					for(x=0;x<8;x++) {
						if (c&128) SDLPlot(sx+x,sy+y,clr);
						else SDLPlot(sx+x,sy+y,clut[3]);
						c=c<<1;
					}
				}
				cw=ch=8;
				break;
			}
		case 3: { /* decenders */
				if ((chr&0x60)==0x60) l=1;
				else l=0;
				for(y=0;y<10;y++) {
					if (l) {
						if (y<2) c=0;
						else if ((y>=2)&&(y<=7))
							c=*(dat+y);
						else c=*(dat-8+y);
					} else {
						if (y<8) c=*(dat+y); else c=0;
					}
					if (chr>=128) c=255-c;
					for(x=0;x<8;x++) {
						if (c&128) SDLPlot(sx+x,sy+y,clr);
						else SDLPlot(sx+x,sy+y,clut[3]);
						c=c<<1;
					}
				}
				cw=8; ch=10;
				break;
			}
		case 4: {
				for(y=0;y<8;y++) {
					c=*(dat+y);
					for(x=0;x<4;x++) {
						l=(c&192)>>6;
						if ((chr>=128)&&(l==3)) l=4;
						if (l) {
							SDLPlot(sx+x*2,sy+y,clut[l]);
							SDLPlot(sx+(x*2+1),sy+y,clut[l]);
						}
						c=c<<2;
					}
				}
				cw=ch=8;
				break;
			}
		case 5: {
				for(y=0;y<8;y++) {
					c=*(dat+y);
					for(x=0;x<4;x++) {
						l=(c&192)>>6;
						if ((chr>=128)&&(l==3)) l=4;
						if (l) {
							SDLPlot(sx+x*2,sy+y*2,clut[l]);
							SDLPlot(sx+(x*2+1),sy+y*2,clut[l]);
							SDLPlot(sx+x*2,sy+y*2+1,clut[l]);
							SDLPlot(sx+(x*2+1),sy+y*2+1,clut[l]);
						}
						c=c<<2;
					}
				}
				cw=8; ch=16;
				break;
			}
		case 6: { /* Gr. 1 */
				for(y=0;y<8;y++) {
					c=*(dat+y);
					for(x=0;x<8;x++) {
						if (c&128) {
							SDLPlot(sx+x*2,sy+y,clut[clr]);
							SDLPlot(sx+(x*2+1),sy+y,clut[clr]);
						}
						c=c<<1;
					}
				}
				cw=16; ch=8;
				break;
			}
		case 7: { /* Gr. 2 */
				for(y=0;y<8;y++) {
					c=*(dat+y);
					for(x=0;x<8;x++) {
						if (c&128) {
							SDLPlot(sx+x*2,sy+y*2,clut[clr]);
							SDLPlot(sx+(x*2+1),sy+y*2,clut[clr]);
							SDLPlot(sx+x*2,sy+y*2+1,clut[clr]);
							SDLPlot(sx+(x*2+1),sy+y*2+1,clut[clr]);
						}
						c=c<<1;
					}
				}
				cw=ch=16;
				break;
			}
		default: {
				 cw=ch=0;
				 txterr("Unknown ANTIC mode requested.\n");
			 }
	}

	if ((mainScreen->zoom==1)&&(SDL_MUSTLOCK(mainScreen->current))) {
		SDL_UnlockSurface(mainScreen->current);
	} else if ((mainScreen->zoom>1)&&(mainScreen->update)) {
		sx*=mainScreen->zoom; sy*=mainScreen->zoom;
		cw*=mainScreen->zoom; ch*=mainScreen->zoom;
		SDL_UpdateRect(mainScreen->current,sx,sy,cw,ch);
	}
	return 1;
}

/*===========================================================================
 * SDLplotchr
 * plot a character in a given ANTIC mode on character screen
 * param sx: x-position
 * param sy: y-position
 * param chr: character to plot
 * param typ: ANTIC mode (+mode 1, which is Gr.0 with no background)
 * param fnt: pointer to font data
 * returns: nothing useful
 *==========================================================================*/
int SDLplotchr(int sx, int sy, int chr, int typ, unsigned char *fnt)
{

	unsigned char clr, *dat;
	dat=fnt+(chr*8);
	if (typ==1) {
		clr=46;
	} else if (typ==2) {
		clr=10;
		typ=1;
	} else
		clr=1;
	return SDLCharEngine(sx,sy,chr,typ,clr,dat);
}

/*===========================================================================
 * SDLmap_plotchr
 * plot a character in a given ANTIC mode on map screen (remaps font banks)
 * param sx: x-position
 * param sy: y-position
 * param chr: character to plot
 * param typ: ANTIC mode (+mode 1, which is Gr.0 with no background)
 * param fnt: pointer to font data
 * returns: nothing useful
 *==========================================================================*/
int SDLmap_plotchr(int sx, int sy, int chr, int typ, unsigned char *fnt)
{
	unsigned char clr,*dat;

	clr=0;
	if ((typ==6)||(typ==7)) {
		dat=fnt+(chr&63)*8+base;
		clr=(chr>>6)+1;
	} else {
		if (typ==1)
			clr=10;
		else if ((typ==2)||(typ==3)) {
			clr=(clut[3]&240)+(clut[2]&15);
		}
		dat=fnt+(chr&127)*8;
	}

	return SDLCharEngine(sx,sy,chr,typ,clr,dat);
}

/*===========================================================================
 * unpack
 * unpack title screen
 * param look: titlescreen data
 * returns: nothing useful
 *==========================================================================*/
int unpack(unsigned char *look)
{
	int i,x,y,c,t;

	SDLClear(0);
	if ((mainScreen->zoom==1)&&(SDL_MUSTLOCK(mainScreen->current))) {
		SDL_LockSurface(mainScreen->current);
	}
	x=44; y=76; c=t=0;
	do {
		if (!t) {
			c=*look++;
			i++;
			if (!c) {
				t=*look++;
				i++;
				t--;
			}
		} else t--;
		SDLPlot(x,y,c);
		x++;
		if (x==253) { x=44; y++; }
	} while(y<130);
	if ((mainScreen->zoom==1)&&(SDL_MUSTLOCK(mainScreen->current))) {
		SDL_UnlockSurface(mainScreen->current);
	}
	return 1;
}

/*===========================================================================
 * SDLBox
 * draw a filled box
 * param x1: upper-left x-pos
 * param y1: upper-left y-pos
 * param x2: lower-right x-pos
 * param y2: lower-right y-pos
 * param clrIdx: color
 * returns: nothing useful
 *==========================================================================*/
int SDLBox(int x1, int y1, int x2, int y2, int clrIdx)
{
	Uint32 clr=mainScreen->clut[clrIdx];
	SDL_Rect rect;

	if (x1>x2) {
		int tmp=x2;
		x2=x1;
		x1=tmp;
	}
	if (y1>y2) {
		int tmp=y2;
		y2=y1;
		y1=tmp;
	}
	rect.x=x1*mainScreen->zoom;
	rect.y=y1*mainScreen->zoom;
	rect.w=(x2+1)*mainScreen->zoom-rect.x;
	rect.h=(y2+1)*mainScreen->zoom-rect.y;
	SDL_FillRect(mainScreen->current,&rect,clr);
	if (mainScreen->update)
		SDL_UpdateRect(mainScreen->current,rect.x,rect.y,rect.w,rect.h);
	return 1;
}

/*===========================================================================
 * SDLXORBox
 * draw a filled box, blitted with XOR color
 * param x1: upper-left x-pos
 * param y1: upper-left y-pos
 * param x2: lower-right x-pos
 * param y2: lower-right y-pos
 * returns: nothing useful
 *==========================================================================*/
int SDLXORBox(int x1, int y1, int x2, int y2)
{
	SDL_Surface *s,*n;
	SDL_Rect rect,xorR;
	int x,y;

	if (x1>x2) {
		int tmp=x2;
		x2=x1;
		x1=tmp;
	}
	if (y1>y2) {
		int tmp=y2;
		y2=y1;
		y1=tmp;
	}
	rect.x=x1*mainScreen->zoom;
	rect.y=y1*mainScreen->zoom;
	rect.w=(x2+1)*mainScreen->zoom-rect.x;
	rect.h=(y2+1)*mainScreen->zoom-rect.y;
	xorR.x=xorR.y=0;

	s=SDL_CreateRGBSurface(SDL_SWSURFACE,rect.w,rect.h,32,rmask, gmask, bmask, amask);
	SDL_BlitSurface(mainScreen->current,&rect,s,&xorR);
	if (SDL_MUSTLOCK(s)) SDL_LockSurface(mainScreen->current);
	for(y=0;y<rect.h;y++) {
		Uint32 look;
		Uint8 *p=(Uint8 *)s->pixels+y*s->pitch;
		for(x=0;x<rect.w;x++) {
			look=*(Uint32 *)p;
			*(Uint32 *)p=look^0xffffff;
			p+=4;
		}
	}
	if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(mainScreen->current);
	n=SDL_DisplayFormat(s);
	SDL_FreeSurface(s);
	SDL_BlitSurface(n,NULL,mainScreen->current,&rect);
	SDL_FreeSurface(n);
	if (mainScreen->update)
		SDL_UpdateRect(mainScreen->current,rect.x,rect.y,rect.w,rect.h);
	return 1;
}

/*===========================================================================
 * SDLXORHollowBox
 * draw a hollow box, blitted with XOR color
 * param x1: upper-left x-pos
 * param y1: upper-left y-pos
 * param x2: lower-right x-pos
 * param y2: lower-right y-pos
 * returns: nothing useful
 *==========================================================================*/
int SDLXORHollowBox(int x1, int y1, int x2, int y2)
{
	SDL_Surface *s,*n;
	SDL_Rect rect,xorR;
	int x,y;

	if (x1>x2) {
		int tmp=x2;
		x2=x1;
		x1=tmp;
	}
	if (y1>y2) {
		int tmp=y2;
		y2=y1;
		y1=tmp;
	}
	rect.x=x1*mainScreen->zoom;
	rect.y=y1*mainScreen->zoom;
	rect.w=(x2+1)*mainScreen->zoom-rect.x;
	rect.h=(y2+1)*mainScreen->zoom-rect.y;
	xorR.x=xorR.y=0;

	s=SDL_CreateRGBSurface(SDL_SWSURFACE,rect.w,rect.h,32,rmask, gmask, bmask, amask);
	SDL_BlitSurface(mainScreen->current,&rect,s,&xorR);
	if (SDL_MUSTLOCK(s)) SDL_LockSurface(mainScreen->current);
	for(y=0;y<rect.h;y++) {
		Uint32 look;
		Uint8 *p=(Uint8 *)s->pixels+y*s->pitch;
		for(x=0;x<rect.w;x++) {
			look=*(Uint32 *)p;
			if ((!x)||(!y)||(y==rect.h-1)||(x==rect.w-1))
				*(Uint32 *)p=look^0xffffff;
			p+=4;
		}
	}
	if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(mainScreen->current);
	n=SDL_DisplayFormat(s);
	SDL_FreeSurface(s);
	SDL_BlitSurface(n,NULL,mainScreen->current,&rect);
	SDL_FreeSurface(n);
	if (mainScreen->update)
		SDL_UpdateRect(mainScreen->current,rect.x,rect.y,rect.w,rect.h);
	return 1;
}

/*===========================================================================
 * SDLHollowBox
 * draw a hollow box
 * param x1: upper-left x-pos
 * param y1: upper-left y-pos
 * param x2: lower-right x-pos
 * param y2: lower-right y-pos
 * param clr: color
 * returns: nothing useful
 *==========================================================================*/
int SDLHollowBox(int x1, int y1, int x2, int y2, int clr)
{
	SDLLine(x1,y1,x2,y1,clr);
	SDLLine(x2,y1,x2,y2,clr);
	SDLLine(x1,y2,x2,y2,clr);
	SDLLine(x1,y1,x1,y2,clr);
	return 1;
}

/*===========================================================================
 * SDLLine
 * draw a vertical or horizontal line
 * param x1: upper-left x-pos
 * param y1: upper-left y-pos
 * param x2: lower-right x-pos
 * param y2: lower-right y-pos
 * param clr: color
 * returns: nothing useful
 *==========================================================================*/
int SDLLine(int x1, int y1, int x2, int y2, int clrIdx)
{
	if ((x1==x2)||(y1==y2)) {
		SDLBox(x1,y1,x2,y2,clrIdx);
	}
	return 1;
}

/*===========================================================================
 * SDLHLine
 * draw a horizontal line
 * param x1: start x-pos
 * param y1: end x-pos
 * param y: y-pos
 * param clr: color
 * returns: nothing useful
 *==========================================================================*/
int SDLHLine(int x1, int x2, int y, int clr)
{
	return SDLLine(x1,y,x2,y,clr);
}

/*===========================================================================
 * SDLHLine
 * draw a horizontal line
 * param x: x-pos
 * param y1: start y-pos
 * param y2: end y-pos
 * param clr: color
 * returns: nothing useful
 *==========================================================================*/
int SDLVLine(int x, int y1, int y2, int clr)
{
	return SDLLine(x,y1,x,y2,clr);
}

/*===========================================================================
 * SDLSetContext
 * change drawing context
 * param id: context to use
 * returns: nothing useful
 *==========================================================================*/
int SDLSetContext(int id)
{
	if ((id>=0)&&(id<5)) {
		mainScreen->current=mainScreen->surfaces[id];
		return 1;
	}
	return 0;
}

/*===========================================================================
 * SDLClear
 * clear screen
 * param clrIdx: the color to use
 * returns: nothing useful
 *==========================================================================*/
int SDLClear(int clrIdx)
{
	Uint32 clr=mainScreen->clut[clrIdx];
	SDL_FillRect(mainScreen->current,NULL,clr);
	return 1;
}

/*===========================================================================
 * SDLgetch
 * wait for a keypress (or mouse click)
 * param click: if set, return when mousebutton is released
 * returns: key pressed
 *==========================================================================*/
int SDLgetch(int click)
{
	int err,done,ch;
	SDL_Event event;

	ch=done=0;

	while(!done) {
		err=SDL_WaitEvent(&event);
		if (!err)
			return 0;

		switch (event.type) {
			case SDL_QUIT: {
					       bye();
					       break;
				       }
			case SDL_KEYDOWN: {
						  ch=event.key.keysym.unicode&0x7f;

						  if (event.key.keysym.sym==SDLK_BACKSPACE)
							  ch=8;
						  else if (event.key.keysym.sym==SDLK_RETURN)
							  ch=13;

						  done=1;
						  break;
					  }
			case SDL_MOUSEBUTTONUP: {
							if ((click)&&(event.button.button))
								done=1;
							break;
						}
			default:
						break;
		}
	}
	return ch;
}

/*===========================================================================
 * SDLrelease
 * wait for a mouse release event
 * returns: nothing useful
 *==========================================================================*/
int SDLrelease()
{
	int err;
	SDL_Event event;

	while(1) {
		err=SDL_WaitEvent(&event);
		if (err)
			return 0;

		switch (event.type) {
			case SDL_MOUSEBUTTONUP: {
							return 1;
							break;
						}
			default:
						break;
		}
	}
}

/*===========================================================================
 * SDLTranslateClick
 * scale a mouse click from screen coordinate to logical coordinate
 * returns: scaled value
 *==========================================================================*/
int SDLTranslateClick(int x)
{
	x=x/mainScreen->zoom;
	return x;
}

/*===========================================================================
 * SDLContextBlt
 * copy graphics from one context to another
 * param dst: destination index
 * param dx: destination upper-left x-pos
 * param dy: destination upper-left y-pos
 * param src: source index
 * param sx1: source upper-left x-pos
 * param sy1: source upper-left y-pos
 * param sx2: source lower-right x-pos
 * param sy2: source lower-right x-pos
 * returns: nothing useful
 *==========================================================================*/
int SDLContextBlt(int dst, int dx, int dy, int src, int sx1, int sy1, int sx2, int sy2)
{
	SDL_Rect srcR,dstR;
	int err;

	srcR.x=sx1*mainScreen->zoom;
	srcR.y=sy1*mainScreen->zoom;
	srcR.w=(sx2+1)*mainScreen->zoom-srcR.x;
	srcR.h=(sy2+1)*mainScreen->zoom-srcR.y;
	dstR.x=dx*mainScreen->zoom;
	dstR.y=dy*mainScreen->zoom;

	err=SDL_BlitSurface(mainScreen->surfaces[src],&srcR,mainScreen->surfaces[dst],&dstR);
	if ((!dst)&&(mainScreen->update))
		SDL_UpdateRect(mainScreen->surfaces[dst],dstR.x,dstR.y,dstR.w,dstR.h);
	return err;
}

/*===========================================================================
 * SDLCharBlt
 * copy a character or tile from buffer to a context
 * param dst: destination index
 * param dx: destination upper-left x-pos
 * param dy: destination upper-left y-pos
 * param idx: the character or tile to blit
 * returns: nothing useful
 *==========================================================================*/
int SDLCharBlt(int dst, int dx, int dy, int idx)
{
	SDL_Rect dstR;
	int err;

	dstR.x=dx*mainScreen->zoom;
	dstR.y=dy*mainScreen->zoom;

	err=SDL_BlitSurface(charTable[idx],NULL,mainScreen->surfaces[dst],&dstR);
	if ((!dst)&&(mainScreen->update))
		SDL_UpdateRect(mainScreen->surfaces[dst],dstR.x,dstR.y,dstR.w,dstR.h);
	return err;
}

/*===========================================================================
 * shutdownSDL
 * clean-up SDL allocations
 * returns: nothing useful
 *==========================================================================*/
int shutdownSDL()
{
	int i;

	freeScreen(mainScreen);
	for(i=0;i<512;i++) {
		if (charTable[i])
			SDL_FreeSurface(charTable[i]);
	}
	return 1;
}

/*===========================================================================
 * toggleFullScreen
 * toggle full screen status
 * returns: nothing useful
 *==========================================================================*/
int toggleFullScreen()
{
	int i;
	SDL_Surface *s;

	mainScreen->fullScreen=!mainScreen->fullScreen;

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_InitSubSystem(SDL_INIT_VIDEO);

	if (!mainScreen->fullScreen) {
		mainScreen->surfaces[0]=SDL_SetVideoMode(320*mainScreen->zoom,200*mainScreen->zoom,0,SDL_SWSURFACE);
	} else {
		mainScreen->surfaces[0]=SDL_SetVideoMode(320*mainScreen->zoom,200*mainScreen->zoom,32,SDL_SWSURFACE|SDL_FULLSCREEN);
	}
	if (!mainScreen->surfaces[0]) {
		fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
		return 0;
	}
	mainScreen->current=mainScreen->surfaces[0];

	if (mainScreen->surfaces[1]) SDL_FreeSurface(mainScreen->surfaces[1]);
	s=SDL_CreateRGBSurface(SDL_SWSURFACE,68*mainScreen->zoom,146*mainScreen->zoom,32,rmask, gmask, bmask, amask);  // upd
	mainScreen->surfaces[1]=SDL_DisplayFormat(s);
	SDL_FreeSurface(s);

	if (mainScreen->surfaces[2]) SDL_FreeSurface(mainScreen->surfaces[2]);
	s=SDL_CreateRGBSurface(SDL_SWSURFACE,256*mainScreen->zoom,32*mainScreen->zoom,32,rmask, gmask, bmask, amask);  // dialog
	mainScreen->surfaces[2]=SDL_DisplayFormat(s);
	SDL_FreeSurface(s);

	if (mainScreen->surfaces[3]) SDL_FreeSurface(mainScreen->surfaces[3]);
	s=SDL_CreateRGBSurface(SDL_SWSURFACE,64*mainScreen->zoom,64*mainScreen->zoom,32,rmask, gmask, bmask, amask);  // back
	mainScreen->surfaces[3]=SDL_DisplayFormat(s);
	SDL_FreeSurface(s);

	// reset palette...
	for(i=0;i<256;i++) {
		int r,g,b;
		r=(mainScreen->rgb[i]>>16)&0xff;
		g=(mainScreen->rgb[i]>>8)&0xff;
		b=mainScreen->rgb[i]&0xff;
		mainScreen->clut[i]=SDL_MapRGB(mainScreen->surfaces[0]->format,r,g,b);
	}
	return 1;
}

/*===========================================================================
 * initSDL
 * initialize graphics layer
 * param zoom: zoom level for screen pixels
 * param fullScreen: flag indicating full screen status
 * returns: flag indicating success
 *==========================================================================*/
int initSDL(int zoom, int fullScreen)
{
	SDL_RWops *src;

	if (SDL_Init(SDL_INIT_VIDEO)<0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		return 0; 
	}
	atexit(SDL_Quit);

	src=SDL_RWFromMem(icon,3126);
	iconW=SDL_LoadBMP_RW(src,1);
	if (iconW) {  /* windows wants this before video init... */
		SDL_WM_SetIcon(iconW,NULL);
	}
	mainScreen=getScreen(zoom,fullScreen);
	if (!mainScreen)
		return 0;
	toggleFullScreen();

	if (iconW) {  /* X11 it after... seem okay to set twice (!) */
		SDL_WM_SetIcon(iconW,NULL);
	}

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_WM_SetCaption("EnvisionPC","EnvisionPC");

	return 1;
}
/*=========================================================================*/
