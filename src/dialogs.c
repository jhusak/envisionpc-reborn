/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: dialogs.c                                                        *
 * Author: Mark Schmelzenbach                                             *
 * Started: 07/28/97                                                      *
 * Long overdue port to SDL 1/26/2006                                     *
 =========================================================================*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "SDL.h"

#include "SDLemu.h"
#include "envision.h"

/*===========================================================================
 * drawbutton
 * draw a push button
 * param x: upper left x-position of button
 * param y: upper left y-position of button
 * param txt: button text
 * returns: nothing useful
 *==========================================================================*/
int drawbutton(int x, int y, char *txt)
{
	int l,o,c;
	char *f,btxt[16];

	btxt[0]=c=o=0;
	f=strchr(txt,'*');
	if (!f)
		strcpy(btxt,txt);
	else {
		if (f-txt) {
			strncpy(btxt,txt,f-txt);
			btxt[f-txt]=0;
		}
		strcat(btxt,f+1);
		c=*(f+1);
		o=(f-txt)*8;
	}
	l=32-strlen(btxt)*4;
	SDLBox(x+1,y+1,x+63,y+8,148);
	SDLLine(x,y,x+64,y,152);
	SDLLine(x+64,y,x+64,y+9,152);
	SDLLine(x,y+9,x+64,y+9,144);
	SDLLine(x,y+1,x,y+9,144);
	SDLstring(x+l,y+1,btxt);
	if (c) {
		SDLplotchr(x+l+o,y+1,c-32,1,dfont);
	}
	return 1;
}


struct dblBufferTmpStorage
{
	int x,y,w,h;
} dblBufferTmpStorage;

/*===========================================================================
 * openDblBufferDialog
 * Stores background
 * draws a raised box
 * param x: upper left x-position of box
 * param y: upper left y-position of box
 * param w: box width
 * param h: box height
 * returns: computed x of the box or passed through
 *==========================================================================*/

int openDblBufferDialog(int x, int y, int w, int h)
{
	
	if (x<0)
	{
		switch (x) {
			case DIALOG_LEFT: x=8; break;
			case DIALOG_RIGHT: x=CONFIG.screenWidth-w-8; break;
			default:
				x=(CONFIG.screenWidth-w)/2;
		}
	}
	
	if (w>MAX_DIALOG_WIDTH-2) w=MAX_DIALOG_WIDTH-2;
	if (h>MAX_DIALOG_HEIGHT-2) h=MAX_DIALOG_HEIGHT-2;
	
	SDLNoUpdate();
	SDLContextBlt(DialogContext,0,0,MainContext,x,y,x+w+2,y+h+2);
	raisedbox(x,y,x+w,y+h);
	dblBufferTmpStorage.x=x;
	dblBufferTmpStorage.y=y;
	dblBufferTmpStorage.w=w+2;
	dblBufferTmpStorage.h=h+2;
	return x;
}

void closeLastDblBufferDialog()
{
	SDLContextBlt(MainContext,dblBufferTmpStorage.x,dblBufferTmpStorage.y,DialogContext,0,0,dblBufferTmpStorage.w,dblBufferTmpStorage.h);
	SDLUpdate();
}

/*===========================================================================
 * raisedbox
 * draw a raised box
 * param x: upper left x-position of box
 * param y: upper left y-position of box
 * param w: box width
 * param h: box height
 * returns: computed x of the box or passed through
 *==========================================================================*/
int raisedbox(int x, int y, int w, int h)
{
	w=w-x;
	h=h-y;
	
	
	SDLBox(x+1,y+1,x+w-1,y+h-1,148);
	SDLLine(x,y,x+w,y,152);
	SDLLine(x+w,y+1,x+w,y+h,144);
	SDLLine(x,y+h,x+w,y+h,144);
	SDLLine(x,y+1,x,y+h,152);
	SDLLine(x+w+1,y+1,x+w+1,y+h+1,0);
	SDLLine(x+w+2,y+2,x+w+2,y+h+2,0);
	SDLLine(x+1,y+h+1,x+w+1,y+h+1,0);
	SDLLine(x+2,y+h+2,x+w+2,y+h+2,0);
	return x;
}

/*===========================================================================
 * stoa
 * convert an ASCII character to ATASCII
 * param s: character to convert
 * returns: the converted character
 *==========================================================================*/
char stoa(char s)
{
	if ((s>=32)&&(s<=96)) s-=32;
	else if ((s<32)&&(s)) s+=64;
	return s;
}

/*===========================================================================
 * ginput
 * handle an input box
 * param xp: upper left x-position of input box
 * param yp: upper left y-position of input box
 * param slen: screen length
 * param len: max input length
 * param init: the default value
 * param tp: 0=normal 1=num 2=filename
 * returns: the user string
 *==========================================================================*/
char *ginput(int xp, int yp, int slen, int len, char *init, int tp)
{
	/* tp 0=normal 1=num 2=filename */

	int cx, cc, dd, cp;
	char buffer[256];
	char c, *r;

	SDL_ShowCursor(SDL_DISABLE);
	SDLBox(xp,yp,slen,yp+8,146);
	SDLHLine(xp-1,slen+1,yp-1,144);
	SDLHLine(xp-1,slen+1,yp+9,152);
	SDLVLine(xp-1,yp,yp+8,144);
	SDLVLine(slen+1,yp,yp+8,152);

	if (init) {
		SDLstring(xp+1,yp,init);
		cp=strlen(init);
		cx=xp+1+8*cp;
		strcpy(buffer,init);
	} else {
		cx=xp+1;
		cp=0;
		*buffer=0;
	}

	cc=10;
	dd=500;

	do {
		SDLVLine(cx,yp,yp+7,cc);
		c=SDLgetch(0);

		if (!c) {
			SDLgetch(0);
		} else
			if ((c==8)&&(cp)) {
				cp--;
				cx-=8;
				SDLBox(cx,yp,cx+9,yp+7,146);
				buffer[cp]=0;
				cc=10;
				dd=500;
			} else if ((cp<len)&&(cx+8<slen)&&((isalnum(c))||(c=='.')
						||(c=='\\')||(c=='/'))) {
				if ((tp==1)&&(isalpha(c))) ;
				else if ((tp==2)&&((c=='/')||(c=='\\')||(cp==12)
							||((c=='.')&&(strchr(buffer,'.'))))) ;
				else {
					if (tp==2) c=toupper(c);
					buffer[cp]=c;
					cp++;
					buffer[cp]=0;
					SDLVLine(cx,yp,yp+7,146);
					SDLplotchr(cx,yp,stoa(c),2,dfont);
					cx+=8;
					cc=10;
					dd=500;
				}
			}
	} while (c!=13);

	SDLVLine(cx,yp,yp+7,146);

	if (*buffer) {
		r=(char *)malloc(strlen(buffer)+1);
		strcpy(r,buffer);
	} else r=NULL;
	SDL_ShowCursor(SDL_ENABLE);
	return r;
}

/*===========================================================================
 * get_filename
 * get a filename from the user
 * param title: the title for the input box
 * param image: the name of the disk image to save to (optional)
 * returns: the filename
 *==========================================================================*/
char *get_filename(char *title, char *image)
{
	char *r;
	int tmpx;
	int tmpy=EDIT_OFFSET_Y;

	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 164, 32);
	SDLstring(tmpx+4,68+tmpy,title);
	SDLUpdate();
	if (image)
		r=ginput(tmpx+4,78+tmpy,tmpx+156,24,NULL,2);
	else
		r=ginput(tmpx+4,78+tmpy,tmpx+156,24,NULL,0);
	closeLastDblBufferDialog();
	return r;
}

/*===========================================================================
 * error_dialog
 * display an error dialog
 * param error: the error message
 * returns: nothing useful
 *==========================================================================*/
int error_dialog(char *error)
{
	int tmpx;
	int tmpy=EDIT_OFFSET_Y+64;
	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 164, 32);
	SDLstring(tmpx+4,68+tmpy,error);
	drawbutton(tmpx+(164-16)/2,78+tmpy,"Okay");
	SDLUpdate();
	SDLgetch(1);
	closeLastDblBufferDialog();
	return 1;
}

/*===========================================================================
 * do_options
 * update the user preferences
 * returns: nothing useful
 *==========================================================================*/
int do_options()
{
	int c,yp;
	char *names[]={"Basic","MAE","Mac/65","Action!","C"};
	char *hot="BM6AC";
	char *oldname=NULL;
	int tmpx;

	yp=EDIT_OFFSET_Y+64;
	
	tmpx=openDblBufferDialog(DIALOG_CENTER, yp, 200, 108);
	SDL_ShowCursor(SDL_DISABLE); /* 144 */
	SDLNoUpdate();
	yp+=2;
	SDLstring(tmpx+28,yp,"Default Options");
	yp+=20;
	SDLstring(tmpx+3,yp,"Use disk image? ( / )");
	SDLplotchr(tmpx+139,yp,57,1,dfont);
	SDLplotchr(tmpx+155,yp,46,1,dfont);
	SDLUpdate();
	do {
		c=toupper(SDLgetch(0));
	} while ((c!='Y')&&(c!='N')&&(c!=27));

	if (c=='Y') {
		SDLNoUpdate();
		SDLBox(tmpx+128,yp,tmpx+181,yp+8,148);
		SDLstring(tmpx+129,yp,"Yes");
		SDLUpdate();
		yp+=10;
		oldname=options.disk_image;
		options.disk_image=ginput(tmpx+3,yp,tmpx+198,24,oldname,0);
		SDLNoUpdate();
		if (oldname)
			free(oldname);
		SDLBox(tmpx+2,yp-1,tmpx+200,yp+9,148);
		if (options.disk_image)
			SDLstring(tmpx+4,yp,options.disk_image);
		else yp-=10;
	} else {
		if (options.disk_image)
			free(options.disk_image);
		options.disk_image=NULL;
	}
	if (!options.disk_image) {
		SDLBox(tmpx+128,yp,tmpx+171,yp+8,148);
		SDLstring(tmpx+129,yp,"No");
		SDLUpdate();
	}
	yp+=16;
	SDLNoUpdate();
	SDLstring(tmpx+3,yp,"Select export format:");
	yp+=8;
	SDLstring(tmpx+11,yp,".BAS .MAE .M65 .ACT .C");
	SDLplotchr(tmpx+19,yp,34,1,dfont);
	SDLplotchr(tmpx+59,yp,45,1,dfont);
	SDLplotchr(tmpx+107,yp,22,1,dfont);
	SDLplotchr(tmpx+139,yp,33,1,dfont);
	SDLplotchr(tmpx+179,yp,35,1,dfont);
	SDLUpdate();
	do {
		c=toupper(SDLgetch(0));
		oldname=strchr(hot,c);
	} while(!oldname);
	c=oldname-hot;
	yp-=8;
	SDLNoUpdate();
	SDLBox(tmpx+3,yp,tmpx+190,yp+16,148);
	SDLstring(tmpx+3,yp,"Export format:");
	SDLstring(tmpx+119,yp,names[c]);
	options.write_tp=c;
	if ((!c)||(c==2)) {
		yp+=10;
		SDLstring(tmpx+11,yp,"Base:");
		SDLUpdate();
		do { oldname=ginput(tmpx+54,yp,tmpx+99,5,NULL,1); } while(!oldname);
		SDLNoUpdate();
		SDLBox(tmpx+53,yp-1,tmpx+100,yp+9,148);
		SDLstring(tmpx+55,yp,oldname);
		options.base=atoi(oldname);
		free(oldname);
		yp+=10;
		SDLstring(tmpx+11,yp,"Step:");
		SDLUpdate();
		do { oldname=ginput(tmpx+54,yp,tmpx+99,5,NULL,1); } while(!oldname);
		SDLNoUpdate();
		SDLBox(tmpx+53,yp-1,tmpx+100,yp+9,148);
		SDLstring(tmpx+55,yp,oldname);
		options.step=atoi(oldname);
		free(oldname);
	} else {
		options.base=options.step=0;
	}
	yp+=20;
	drawbutton(tmpx+67,yp,"Okay");
	SDLUpdate();
	SDLgetch(1);
	SDL_ShowCursor(SDL_ENABLE);
	closeLastDblBufferDialog();
	return 1;
}

int palette_click(int i, int mx, int my)
{
	int lum, col;
	
	SDLNoUpdate();
	if (IN_BOX(mx-EDIT_COLOR_X ,my-EDIT_COLOR_Y,i*44-31,i*44+96,-96,15)) {
		lum=(mx-EDIT_COLOR_X-i*44+31)/16;
		col=(my-EDIT_COLOR_Y+96)/7;
		col=col*16+lum*2;
	}
	else
		col=-1;
		SDLUpdate();

	return col;
}

int show_palette(i,x,y)
{
	
	openDblBufferDialog(x-32,y-130,129,113);
	SDL_Event event;
	int done=0;
	int col,lum;
	
	for (col=0; col<16; col++)
		for (lum=0; lum<8; lum++)
		{
			SDLBox(x+lum*16+1-32, y+col*7-129, x+lum*16+16-32, y+col*7+6-129, col*16+lum*2);
			if (clut[i]==col*16+lum*2)
				SDLHollowBox(x+lum*16+1-32, y+col*7-129, x+lum*16+16-32, y+col*7+6-129, 14);
			
		}
	SDLUpdate();
	
	do {
		SDL_WaitEvent(&event);
		
		switch(event.type){  /* Process the appropiate event type */
			case SDL_QUIT: {
				command(0,SDLK_ESCAPE);
				break;
			}
			case SDL_KEYDOWN: {
				int sym=event.key.keysym.sym;
				if ((sym==SDLK_ESCAPE)) {
					col=-1;
					done=1;
				} 
				break;
			}
			case SDL_MOUSEBUTTONDOWN: {
				int mx,my;
				mx=SDLTranslateClick(event.button.x);
				my=SDLTranslateClick(event.button.y);
				col=palette_click(i,mx,my);
				done=1;
				break;
			}
			default:
				break;
		}
	} while(!done);
	
	if (col>=0)
		clut[i]=col;
	
	closeLastDblBufferDialog();
}

/*===========================================================================
 * do_colors
 * update the color registers
 * returns: nothing useful
 *==========================================================================*/
int do_colors()
{
	int i,yp;
	char buf[16],*color;
	int tmpx;

	yp=64+EDIT_OFFSET_Y;
	tmpx = openDblBufferDialog(DIALOG_CENTER,yp, 178, 96);
	SDL_ShowCursor(SDL_DISABLE);
	yp+=2;
	SDLstring(tmpx+18,yp,"Color Registers");
	SDLUpdate();
	yp+=10;
	for(i=0;i<5;i++) {
		SDLNoUpdate();
		SDLHollowBox(tmpx+11,yp,tmpx+21,yp+8,0);
		SDLBox(tmpx+12,yp+1,tmpx+20
			   ,yp+7,clut[i]);
		sprintf(buf,"PF%d:",i);
		SDLstring(tmpx+27,yp,buf);
		SDLUpdate();

		sprintf(buf,"%d",clut[i]);
		do { color=ginput(tmpx+62,yp,tmpx+89,5,buf,1); } while(!color);
		SDLNoUpdate();

		SDLBox(tmpx+61,yp-1,tmpx+100,yp+9,148);
		SDLstring(tmpx+63,yp,color);
		clut[i]=(atoi(color)&254);
		SDLBox(tmpx+12,yp+1,tmpx+17,yp+7,clut[i]);
		SDLUpdate();

		free(color);
		yp+=10;
	}
	yp+=10;
	drawbutton(tmpx+57,yp,"Okay");
	SDLgetch(1);
	SDL_ShowCursor(SDL_ENABLE);
	closeLastDblBufferDialog();
	return 1;
}

int get_8x8_mode(int m)
{
	switch (m) {
		case 1: return 1;
		case 4:
		case 5: return 4;
		default:
			return 2;
	}
	return 2;
}


/*===========================================================================
 * select_draw
 * get a draw character from the user
 * param title: title of the dialog box
 * returns: the selected character
 *==========================================================================*/
int select_draw(char *title)
{
	int i,sx,sy,yp,clr;
	int m=get_8x8_mode(mode);
    char * dat;
	SDL_Event evt;

	SDLNoUpdate();
	raisedbox(24,32,296,120);
	SDLstring(33,36,title);
	yp=104;
	SDLBox(31,48,289,yp+8,clut[0]);
	SDLHLine(30,290,47,144);
	SDLHLine(30,290,yp+9,152);
	SDLVLine(30,48,yp+8,144);
	SDLVLine(290,48,yp+8,152);
	sx=0; sy=0;
	
	for(i=0;i<256;i++) {
		if (mode<6) {
			SDLmap_plotchr(32+sx,48+sy,i,m,font);
		} else {
			dat=(char *)(font+(i&63)*8+base);
			clr=(i>>6)+1;
			SDLCharEngine(32+sx,48+sy,i&127,1,clut[clr],(unsigned char *)dat);
		}
		sx+=8;
		if (sx>248) { sx=0; sy+=8; }
	}
	SDLUpdate();

	yp=0;
	do {
		SDL_WaitEvent(&evt);

		switch(evt.type){
			case SDL_KEYDOWN: {
						  i=stoa(evt.key.keysym.unicode&0x7f);
						  if ((i)||((evt.key.keysym.unicode==' '))) {
							  if ((evt.key.keysym.mod&KMOD_ALT))
								  i=i+128;
							  yp=1;
						  }
					  }
			case SDL_MOUSEBUTTONUP: {
							if (evt.button.button==1) {
								int mx,my;
								mx=SDLTranslateClick(evt.button.x);
								my=SDLTranslateClick(evt.button.y);
								if ((mx>=31)&&(mx<=289)&&(my>=48)&&(my<=112)) {
									sx=(mx-32)/8; sy=(my-48)/8;
									i=sy*32+sx;
									yp=1;
								}
							}
						}
			default:
						break;
		}
	} while (!yp);
	return i;
}

/*===========================================================================
 * get_number
 * get a number from the user
 * param title: title of the dialog box
 * param def: the default value
 * param max: the max allowable value
 * returns: the number
 *==========================================================================*/
int get_number(char *title, int def, int max)
{
	char *r, buf[8];
	int i;

	int tmpx;
	int tmpy=EDIT_OFFSET_Y;
	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 164, 32);
	SDLstring(tmpx+4,68+tmpy,title);
	SDLUpdate();

	if (def>=0)
		sprintf(buf,"%d",def);
	else *buf=0;
	do {
		r=ginput(tmpx+4,80+tmpy,tmpx+156,24,buf,1);
		if (r) {
			i=atoi(r);
			free(r);
		} else i=-1;
	} while ((max>=0)&&(i>max));
	closeLastDblBufferDialog();
	return i;
}

/*===========================================================================
 * do_size
 * resize a map
 * param tileMode: flag indicating 0: map mode; 1 tile mode;
 * returns: nothing useful
 *==========================================================================*/
int do_size(int tileMode)
{
	int i, x, y, loop, tmax;
	char buf[16], *size;
	unsigned char *newmap,*look;
	if (tileMode) {
		tileMode=8; tmax=16;
	} else tmax=65536;

	int tmpx;
	int tmpy=EDIT_OFFSET_Y;
	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 160, 32);
	
	if (!tileMode) {
		SDLstring(tmpx+4,68+tmpy,"New Width:");
		sprintf(buf,"%d",map->w);
	} else {
		SDLstring(tmpx+4,68+tmpy,"Tile Width:");
		sprintf(buf,"%d",tsx);
	}
	SDLUpdate();
	loop=1;
	do {
		size=ginput(tmpx+92+4+tileMode,68+tmpy,tmpx+92+64-tileMode,6,buf,1);
		if (size) {
			x=atoi(size);
			if ((x)&&(x<=tmax))
				loop=0;
		} else loop=1;
	} while(loop);
	SDLNoUpdate();
	SDLBox(tmpx+4+91,67+tmpy,tmpx+93+64-tileMode,77+tmpy,148);
	SDLstring(tmpx+4+93+tileMode,68+tmpy,size);
	if (!tileMode) {
		SDLstring(tmpx+4,78+tmpy,"New Height:");
		sprintf(buf,"%d",map->h);
	} else {
		SDLstring(tmpx+4,78+tmpy,"Tile Height:");
		sprintf(buf,"%d",tsy);
	}
	SDLUpdate();
	loop=1;
	do {
		size=ginput(tmpx+4+92+tileMode,78+tmpy,tmpx+92+64-tileMode,6,buf,1);
		if (size) {
			y=atoi(size);
			if ((y)&&(y<=tmax))
				loop=0;
		} else loop=1;
	} while(loop);
	
	closeLastDblBufferDialog();

	if (tileMode) {
		tile_size(x,y);
		return 0;
	}
	if ((map->w==x)&&(map->h==y))
		return 0;
	look=newmap=map->map;
	if (x<map->w) {
		for(loop=0;loop<map->h;loop++) {
			for(i=0;i<x;i++)
				*look++=*newmap++;
			newmap+=map->w-x;
		}
	}
	newmap=(unsigned char *)realloc(map->map,x*y);
	if (newmap) {
		if (x>map->w) {
			map->map=newmap;
			look=newmap+x*y-map->w*map->h;
			memmove(look,newmap,map->w*map->h);
			for(loop=0;loop<map->h;loop++)
				for(i=0;i<x;i++)
					if (i<map->w) *map->map++=*look++;
					else *map->map++=0;
		}
		if (x*y>map->w*map->h)
			memset(newmap+map->w*map->h,0,x*y-map->w*map->h);
		map->map=newmap;
		map->w=x;
		map->h=y;
	} else {
		error_dialog("Cannot allocate new map");
	}
	map->cx=map->cy=map->scx=map->scy=0;
	return 0;
}

/*===========================================================================
 * do_move
 * jump to a screen position
 * param map: the map to update
 * param tileMode: flag indicating 0: map mode; 1 tile mode;
 * returns: nothing useful
 *==========================================================================*/
int do_move(view *map, int tileMode)
{
	int mw, mh, x, y, loop;
	char buf[16], *size;

	int tmpx;
	int tmpy=EDIT_OFFSET_Y;

	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 168, 32);
	
	if (tileMode) {

		SDLstring(tmpx+4,68+tmpy,"Go to tile:");
		SDLUpdate();
		int tx, ty;
		tx=((currentView->scx+currentView->cx)/tsx);
		ty=((currentView->scy+currentView->cy)/tsy);
		sprintf(buf,"%d",tx+ty*16);
		loop=1;
		do {
			size=ginput(tmpx+100,68+tmpy,tmpx+164,6,buf,1);
			if (size) {
				x=atoi(size);
				if (IN_RANGE(x,0,255))
					loop=0;
			} else loop=1;
		} while(loop);
		y=x/16; x=x-y*16;
		x=x*tsx; y=y*tsy;
	} else {
		SDLstring(tmpx+4,68+tmpy,"Move to X:");
		SDLUpdate();
		sprintf(buf,"%d",map->scx+map->cx);
		loop=1;
		do {
			size=ginput(tmpx+92,68+tmpy,tmpx+92+64,6,buf,1);
			if (size) {
				x=atoi(size);
				if (IN_RANGE(x,0,map->w-1))
					loop=0;
			} else loop=1;
		} while(loop);
		SDLNoUpdate();
		SDLBox(tmpx+91,67+tmpy,tmpx+92+65,77+tmpy,148);
		SDLstring(tmpx+93,68+tmpy,size);
		SDLstring(tmpx+4,78+tmpy,"Move to Y:");
		SDLUpdate();
		sprintf(buf,"%d",map->scy+map->cy);
		loop=1;
		do {
			size=ginput(tmpx+92,78+tmpy,tmpx+92+64,6,buf,1);
			if (size) {
				y=atoi(size);
				if ((y>=0)&&(y<map->h))
					loop=0;
			} else loop=1;
		} while(loop);
	}

	closeLastDblBufferDialog();
	
	/*
	   if (mode<6) mw=40;
	   else mw=20;
	   if ((mode==7)||(mode==5)) mh=11;
	   else if (mode==3) mh=18; 
	   else mh=22;
	 */
	mw=320/map->cw;
	mh=176/map->ch;
	if (x<mw) {
		map->scx=0;
	} else if (x>map->w-mw) {
		map->scx=map->w-mw;
	} else {
		map->scx=x-(mw>>1);
	}
	map->cx=x-map->scx;

	if (y<mh) {
		map->scy=0;
	} else if (y>map->h-mh) {
		map->scy=map->h-mh;
	} else {
		map->scy=y-(mh>>1);
	}
	map->cy=y-map->scy;

	return 1;
}

/*===========================================================================
 * do_exit
 * verify a user actually wants to exit
 * returns: flag indicating exit should occur
 *==========================================================================*/
int do_exit()
{
	int ch;
	int x;

	int tmpy=EDIT_OFFSET_Y;
	x=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 164, 32);
	SDLstring(x+4+16,76+tmpy,"Really Exit? Y/N");
	SDLplotchr(x+4+128-8,76+tmpy,stoa('Y'),1,dfont);
	SDLUpdate();
	ch=SDLgetch(1);
	closeLastDblBufferDialog();
	return ((ch=='Y')||(ch=='y'));

}
/*=========================================================================*/