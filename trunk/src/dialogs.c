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

/*===========================================================================
 * raisedbox
 * draw a raised box
 * param x: upper left x-position of box
 * param y: upper left y-position of box
 * param w: box width
 * param h: box height
 * returns: nothing useful
 *==========================================================================*/
int raisedbox(int x, int y, int w, int h)
{
  w=w-x;
  h=h-y;
  SDLBox(x+1,y+1,x+w-1,y+h-1,148);
  SDLLine(x,y,x+w,y,152);
  SDLLine(x+w,y,x+w,y+h,152);
  SDLLine(x,y+h,x+w,y+h,144);
  SDLLine(x,y+1,x,y+h,144);
  return 1;
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
	SDLNoUpdate();
	SDLContextBlt(DialogContext,0,0,MainContext,79,64,243,91);
	raisedbox(79,64,241,90);
	SDLLine(79,91,241,91,0);
	SDLLine(242,65,242,91,0);
	SDLLine(243,66,243,91,0);
	SDLstring(83,68,title);
	SDLUpdate();
	if (image)
		r=ginput(83,78,237,24,NULL,2);
	else
		r=ginput(83,78,237,24,NULL,0);
	SDLContextBlt(MainContext,79,64,DialogContext,0,0,164,27);
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
	SDLNoUpdate();
	SDLContextBlt(DialogContext,0,0,MainContext,79,64,243,91);
	raisedbox(79,64,241,90);
	SDLLine(79,91,241,91,0);
	SDLLine(242,65,242,91,0);
	SDLLine(243,66,243,91,0);
	SDLstring(83,68,error);
	drawbutton(129,78,"Okay");
	SDLUpdate();
	SDLgetch(1);
	SDLContextBlt(MainContext,79,64,DialogContext,0,0,164,27);
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
	char *names[]={"Basic","MAE","Mac/65","Action!"};
	char *hot="BM6A";
	char *oldname=NULL;
	
	SDL_ShowCursor(SDL_DISABLE); /* 144 */
	SDLNoUpdate();
	raisedbox(79,16,258,112);
	SDLstring(107,18,"Default Options");
	yp=30;
	SDLstring(82,yp,"Use disk image? ( / )");
	SDLplotchr(218,yp,57,1,dfont);
	SDLplotchr(234,yp,46,1,dfont);
	SDLUpdate();
	do {
		c=toupper(SDLgetch(0));
	} while ((c!='Y')&&(c!='N')&&(c!=27));
	
	if (c=='Y') {
		SDLNoUpdate();
		SDLBox(210,yp,255,yp+8,148);
		SDLstring(206,yp,"Yes");
		SDLUpdate();
		yp+=10;
		oldname=options.disk_image;
		options.disk_image=ginput(99,yp,253,24,oldname,0);
		SDLNoUpdate();
		if (oldname)
			free(oldname);
		SDLBox(98,yp-1,254,yp+9,148);
		if (options.disk_image)
			SDLstring(99,yp,options.disk_image);
		else yp-=10;
	} else {
		if (options.disk_image)
			free(options.disk_image);
		options.disk_image=NULL;
	}
	if (!options.disk_image) {
		SDLBox(206,yp,255,36,148);
		SDLstring(206,yp,"No");
		SDLUpdate();
	}
	yp+=16;
	SDLNoUpdate();
	SDLstring(82,yp,"Select export format:");
	yp+=8;
	SDLstring(90,yp,".BAS .MAE .M65 .ACT");
	SDLplotchr(98,yp,34,1,dfont);
	SDLplotchr(138,yp,45,1,dfont);
	SDLplotchr(186,yp,22,1,dfont);
	SDLplotchr(218,yp,33,1,dfont);
	SDLUpdate();
	do {
		c=toupper(SDLgetch(0));
		oldname=strchr(hot,c);
	} while(!oldname);
	c=oldname-hot;
	yp-=8;
	SDLNoUpdate();
	SDLBox(82,yp,254,yp+16,148);
	SDLstring(82,yp,"Export format:");
	SDLstring(198,yp,names[c]);
	options.write_tp=c;
	if ((!c)||(c==2)) {
		yp+=10;
		SDLstring(90,yp,"Base:");
		SDLUpdate();
		do { oldname=ginput(135,yp,178,5,NULL,1); } while(!oldname);
		SDLNoUpdate();
		SDLBox(134,yp-1,179,yp+9,148);
		SDLstring(136,yp,oldname);
		options.base=atoi(oldname);
		free(oldname);
		yp+=10;
		SDLstring(90,yp,"Step:");
		SDLUpdate();
		do { oldname=ginput(135,yp,178,5,NULL,1); } while(!oldname);
		SDLNoUpdate();
		SDLBox(134,yp-1,179,yp+9,148);
		SDLstring(136,yp,oldname);
		options.step=atoi(oldname);
		free(oldname);
	} else {
		options.base=options.step=0;
	}
	drawbutton(136,94,"Okay");
	SDLUpdate();
	SDLgetch(1);
	SDLNoUpdate();
	SDLBox(79,16,258,112,0);
	SDLstring(192,104,"Char");
	SDLUpdate();
	SDL_ShowCursor(SDL_ENABLE);
	return 1;
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
	SDLNoUpdate();
	SDL_ShowCursor(SDL_DISABLE);
	raisedbox(79,16,258,112);
	SDLstring(107,18,"Color Registers");
	SDLUpdate();
	yp=30;
	for(i=0;i<5;i++) {
		SDLNoUpdate();
		
		SDLHollowBox(90,yp,100,yp+8,0);
		SDLBox(91,yp+1,99,yp+7,clut[i]);
		sprintf(buf,"PF%d:",i);
		SDLstring(106,yp,buf);
		SDLUpdate();
		
		sprintf(buf,"%d",clut[i]);
		do { color=ginput(151,yp,178,5,buf,1); } while(!color);
		SDLNoUpdate();
		
		SDLBox(150,yp-1,179,yp+9,148);
		SDLstring(152,yp,color);
		clut[i]=(atoi(color)&254);
		SDLBox(91,yp+1,99,yp+7,clut[i]);
		SDLUpdate();
		
		free(color);
		yp+=10;
	}
	drawbutton(136,94,"Okay");
	SDLgetch(1);
	SDLBox(79,16,258,112,0);
	SDLstring(192,104,"Char");
	SDL_ShowCursor(SDL_ENABLE);
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
	int i,sx,sy,yp;
	int m=get_8x8_mode(mode);
	
  SDL_Event evt;

  SDLNoUpdate();
  raisedbox(24,32,296,120);
  SDLstring(33,36,title);
  yp=104;
  SDLBox(31,48,289,yp+8,146);
  SDLHLine(30,290,47,144);
  SDLHLine(30,290,yp+9,152);
  SDLVLine(30,48,yp+8,144);
  SDLVLine(290,48,yp+8,152);
  sx=0; sy=0;

  for(i=0;i<256;i++) {
    SDLmap_plotchr(32+sx,48+sy,i,m,font);
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
	
	SDLContextBlt(DialogContext,0,0,MainContext,79,64,243,91);
	SDLNoUpdate();
	raisedbox(79,64,241,90);
	SDLLine(79,91,241,91,0);
	SDLLine(242,65,242,91,0);
	SDLLine(243,66,243,91,0);
	SDLstring(83,68,title);
	SDLUpdate();
	
	if (def>=0)
		sprintf(buf,"%d",def);
	else *buf=0;
	do {
		r=ginput(83,78,237,24,buf,1);
		if (r) {
			i=atoi(r);
			free(r);
		} else i=-1;
	} while ((max>=0)&&(i>max));
	SDLContextBlt(MainContext,79,64,DialogContext,0,0,164,27);
	return i;
}

/*===========================================================================
 * do_size
 * resize a map
 * param tmode: flag indicating 0: map mode; 1 tile mode;
 * returns: nothing useful
 *==========================================================================*/
int do_size(int tmode)
{
	int i, x, y, loop, tmax;
	char buf[16], *size;
	unsigned char *newmap,*look;
	
	if (tmode) {
		tmode=8; tmax=16;
	} else tmax=65536;
	
	SDLContextBlt(DialogContext,0,0,MainContext,79,64,243,91);
	SDLNoUpdate();
	raisedbox(79,64,241,90);
	SDLLine(79,91,241,91,0);
	SDLLine(242,65,242,91,0);
	SDLLine(243,66,243,91,0);
	
	if (!tmode) {
		SDLstring(83,68,"New Width:");
		sprintf(buf,"%d",map->w);
	} else {
		SDLstring(83,68,"Tile Width:");
		sprintf(buf,"%d",tsx);
	}
	SDLUpdate();
	loop=1;
	do {
		size=ginput(167+tmode,68,210-tmode,6,buf,1);
		if (size) {
			x=atoi(size);
			if ((x)&&(x<=tmax))
				loop=0;
		} else loop=1;
	} while(loop);
	SDLNoUpdate();
	SDLBox(166,67,211+tmode,77,148);
	SDLstring(167,68,size);
	if (!tmode) {
		SDLstring(83,78,"New Height:");
		sprintf(buf,"%d",map->h);
	} else {
		SDLstring(83,78,"Tile Height:");
		sprintf(buf,"%d",tsy);
	}
	SDLUpdate();
	loop=1;
	do {
		size=ginput(175+tmode,78,218-tmode,6,buf,1);
		if (size) {
			y=atoi(size);
			if ((y)&&(y<=tmax))
				loop=0;
		} else loop=1;
	} while(loop);
	SDLContextBlt(MainContext,79,64,DialogContext,0,0,164,27);
	
	if (tmode) {
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
 * param tmode: flag indicating 0: map mode; 1 tile mode;
 * returns: nothing useful
 *==========================================================================*/
int do_move(view *map, int tmode)
{
	int mw, mh, x, y, loop;
	char buf[16], *size;
	
	SDLContextBlt(DialogContext,0,0,MainContext,79,64,243,91);
	SDLNoUpdate();
	raisedbox(79,64,241,90);
	SDLLine(79,91,241,91,0);
	SDLLine(242,65,242,91,0);
	SDLLine(243,66,243,91,0);
	
	if (tmode) {
		
		SDLstring(83,68,"Go to tile:");
		SDLUpdate();
		sprintf(buf,"%d",map->scx+map->cx);
		loop=1;
		do {
			size=ginput(167+8,68,210,6,buf,1);
			if (size) {
				x=atoi(size);
				if ((x>=0)&&(x<256))
					loop=0;
			} else loop=1;
		} while(loop);
		y=x/16; x=x-y*16;
		x=x*tsx; y=y*tsy;
	} else {
		SDLstring(83,68,"Move to X:");
		SDLUpdate();
		sprintf(buf,"%d",map->scx+map->cx);
		loop=1;
		do {
			size=ginput(167,68,210,6,buf,1);
			if (size) {
				x=atoi(size);
				if ((x>=0)&&(x<map->w))
					loop=0;
			} else loop=1;
		} while(loop);
		SDLNoUpdate();
		SDLBox(166,67,211,77,148);
		SDLstring(167,68,size);
		SDLstring(83,78,"Move to Y:");
		SDLUpdate();
		sprintf(buf,"%d",map->scy+map->cy);
		loop=1;
		do {
			size=ginput(167,78,210,6,buf,1);
			if (size) {
				y=atoi(size);
				if ((y>=0)&&(y<map->h))
					loop=0;
			} else loop=1;
		} while(loop);
	}
	
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
	
	SDLContextBlt(MainContext,79,64,DialogContext,0,0,164,27);
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
	SDLNoUpdate();
	
	SDLContextBlt(DialogContext,0,0,MainContext,79,64,243,91);
	raisedbox(79,64,241,90);
	SDLLine(79,91,241,91,0);
	SDLLine(242,65,242,91,0);
	SDLLine(243,66,243,91,0);
	
	SDLstring(83+16,68,"Really Exit? Y/N");
	SDLplotchr(83+128-8,68,stoa('Y'),1,dfont);
	SDLUpdate();
	ch=SDLgetch(1);
	SDLContextBlt(MainContext,79,64,DialogContext,0,0,164,27);
	// this is bad idea
	// return (!((ch=='n')||(ch=='N')));
	return ((ch=='Y')||(ch=='y'));
	
}
/*=========================================================================*/
