/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: envision.c                                                       *
 * Author: Mark Schmelzenbach                                             *
 * Started: 07/28/97                                                      *
 * Long overdue port to SDL 1/26/2006                                     *
 =========================================================================*
 * Todo                                                                   *
 *  - enable toggle fullScreen                                            *
 *  - GTIA and native ANTIC 4,5 editing                                   *
 *  - P/M overlay                                                         *
 *  - preview tiles, moveable map zone                                    *
 =========================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "SDL.h"

#include "SDLemu.h"
#include "colours.h"
#include "font.h"
#include "envision.h"

unsigned char *dfont, *font, *copy_from, *fontbank[10];
char bank_mod[10];
int echr, bank, copy_size, values;

int cmds[13];
unsigned char undo[8];
unsigned char peek[64], plot[64];
opt options;
unsigned char clut[9]={0,40,202,148,70,0,0,0,0};
int orit[8]={128,64,32,16,8,4,2,1};
int andit[8]={127,191,223,239,247,251,253,254};

int corner(int all);
/*=========================================================================*
 * Define rotation look-up tables.
 * The rotate routine is taken from a Graphics Gems II article by
 * Ken Yap (Centre for Spatial Information Systems, CSIRO DIT, Australia)
 * entitled "A Fast 90-Degree Bitmap Rotator" pp. 84-85
 * Code from this article is in the routine rotate8x8()
 *=========================================================================*/
typedef unsigned long bit32;

#define table(name,n)\
  static bit32 name[16]={\
    0x00000000<<n, 0x00000001<<n, 0x00000100<<n, 0x00000101<<n,\
    0x00010000<<n, 0x00010001<<n, 0x00010100<<n, 0x00010101<<n,\
    0x01000000<<n, 0x01000001<<n, 0x01000100<<n, 0x01000101<<n,\
    0x01010000<<n, 0x01010001<<n, 0x01010100<<n, 0x01010101<<n,\
  };
table(ltab0,7);
table(ltab1,6);
table(ltab2,5);
table(ltab3,4);
table(ltab4,3);
table(ltab5,2);
table(ltab6,1);
table(ltab7,0);

/*===========================================================================
 * txterr
 * display an error and exit
 * param txt: error to display
 *==========================================================================*/
void txterr(char *txt)
{
  shutdownSDL();
  if (txt)
    printf("Fatal Error: %s",txt);
  printf("\n\nPress any key to continue...\n");
  exit(1);
}

/*===========================================================================
 * setpal
 * set the Atari palette
 *==========================================================================*/
void setpal()
{
  int x;

  for(x=0;x<256;x++) {
    SDLsetPalette(x,(colortable[x]>>16)&0xff,(colortable[x]>>8)&0xff,(colortable[x])&0xff);
  }
}

/*===========================================================================
 * bye
 * confirm user exit, then exit (possibly)
 *==========================================================================*/
void bye(void)
{
  if (do_exit()) {
    shutdownSDL();
    exit(0);
  } else
    return;
}

/*===========================================================================
 * topos
 * convert character id to character position
 * param c: character id
 * param sx: updated screen position
 * param sy: updated screen position
 * returns: nothing useful
 *==========================================================================*/
int topos(int c, int *sx, int *sy)
{
  *sx=(c%32)*8+8;
  *sy=160+(c>>5)*8;
  return 1;
}

/*===========================================================================
 * grid
 * update the character grid
 * param chr: character id
 * param rem: flag indicating to remember (put into undo buffer)
 * returns: flag indicating character change
 *==========================================================================*/
int grid(int chr, int rem)
{
  unsigned char *dat,c;
  char num[16];
  int x,y,clr;

  if (chr==echr) return 0;

  SDLNoUpdate();
  dat=font+(chr*8);

  if (rem) {
    memcpy(undo,dat,8);
  }

  for(y=0;y<8;y++) {
    c=*(dat+y);
    for(x=0;x<8;x++) {
      if ((x+y)&1) clr=144; else clr=148;
      if (c&128) clr=10;
      SDLBox(192+x*8,32+y*8,192+x*8+7,39+y*8,clr);
      c=c<<1;
    }
  }
  topos(echr,&x,&y);
  SDLBox(x,y,x+7,y+7,0);
  SDLplotchr(x,y,echr,2,font);
  echr=chr;
  topos(echr,&x,&y);
  SDLBox(x,y,x+7,y+7,148);
  SDLplotchr(x,y,echr,2,font);

  SDLBox(264,32,280,64,0);
  SDLplotchr(264,32,echr,6,font);
  SDLplotchr(264,40,echr,7,font);
  SDLplotchr(264,56,echr,4,font);

  SDLBox(224,104,272,111,0);
  sprintf(num,": %d",echr);
  SDLstring(224,104,num);
  SDLplotchr(264,104,echr,2,dfont);
  corner(0);
  SDLUpdate();
  return 1;
}

/*===========================================================================
 * panel
 * update command panel
 * param tp: the panel to show
 * returns: flag indicating character change
 *==========================================================================*/
int panel(int tp)
{
  SDLSetContext(UpdContext);
  SDLClear(0);
  switch(tp) {
    case 1: {
      SDLBox(14,2,25,7,147);
      SDLplotchr(16,2,36,2,dfont);
      drawbutton(0,8,"*Blank"); cmds[1]='b';
      drawbutton(0,18,"*Inverse"); cmds[2]='i';
      drawbutton(0,28,"*Undo"); cmds[3]='u';
      drawbutton(0,38,"*Atari"); cmds[4]='a';
      drawbutton(0,48,"Flip *H"); cmds[5]='h';
      drawbutton(0,58,"Flip *V"); cmds[6]='v';
      drawbutton(0,68,"*Rotate"); cmds[7]='r';
      drawbutton(0,78,"*Copy"); cmds[8]='c';
      drawbutton(0,88,"*X-copy"); cmds[9]='x';
      drawbutton(0,98,"*T-copy"); cmds[10]='t';
      drawbutton(0,108,"*Poke"); cmds[11]='p';

      SDLBox(1,1,12,8,148);
      SDLLine(0,1,0,8,144);
      SDLLine(0,0,13,0,152);
      SDLLine(13,1,13,7,152);
      SDLplotchr(3,1,37,2,dfont);
      cmds[0]=140;
      break;
    }
    case 2: {
      SDLBox(1,2,12,7,147);
      SDLplotchr(3,2,37,2,dfont);
      drawbutton(0,8,"Restore"); cmds[1]='A';
      drawbutton(0,18,"*Save"); cmds[2]='s';
      drawbutton(0,28,"*Load"); cmds[3]='l';
      drawbutton(0,38,"*Export"); cmds[4]='e';
      drawbutton(0,48,"*Options"); cmds[5]='o';
      SDLBox(14,1,24,8,148);
      SDLLine(13,1,13,8,144);
      SDLLine(13,0,25,0,152);
      SDLLine(25,1,25,7,152);
      SDLplotchr(16,1,36,2,dfont);
      cmds[0]=82;
      break;
    }
  }
  SDLSetContext(MainContext);
  SDLContextBlt(MainContext,8,24,UpdContext,0,0,64,120);
  return 1;
}

/*===========================================================================
 * corner
 * update the map preview
 * param all: a flag(?)
 * returns: nothing useful
 *==========================================================================*/
int corner(int all)
{
  int x,y,w,h,f;
  int ox;
  unsigned char *check, *look, i, match, msk, rot, idx;

  f=0;
  rot=6;
  if ((mode==4)||(mode==3)||(mode==2)||(mode==5)) {
    w=h=8;
    if (mode==3) h=10;
    else if (mode==5) h=16;
    msk=127;
    rot=7;
  } else if (mode==7) {
    w=h=16; msk=63;
  } else { w=16; h=8; msk=63; }

  match=echr&msk;

  if (all) {
    SDLSetContext(BackContext);
    SDLBox(0,0,63,63,clut[0]);
  } else {
    SDLSetContext(UpdContext);
    SDLBox(0,0,w*4,h,clut[0]);
    SDLmap_plotchr(0,0,match,mode,font);
    SDLmap_plotchr(w,0,match+64,mode,font);
    SDLmap_plotchr(w*2,0,match+128,mode,font);
    SDLmap_plotchr(w*3,0,match+192,mode,font);
  }

  memset(peek,0,64);
  memset(plot,0,64);
  i=0;
  for(y=0;y<8;y++) {
    if (y>=currentView->h)
      break;
    look=&currentView->map[y*currentView->w];
    i=y*8;
    for(x=0;x<8;x++) {
      if (x>=currentView->w)
	break;
      peek[i]=*look++;
      plot[i++]=1;
    }
  }
  if ((tsx>1)||(tsy>1)) {
    unsigned char work[128];

    memcpy(work,peek,64);
    memcpy(work+64,plot,64);
    memset(plot,0,64);
    i=0;
    for(y=0;y<8*tsy;y+=tsy) {
      for(x=0;x<8*tsx;x+=tsx) {
	int sx,sy,tx,ty;
	if (!work[64+i]) {
	  i++;
	  continue;
	}
	ty=work[i]/16; tx=work[i]-ty*16;
	i++;
	look=tile->map+tx*tsx+ty*tsy*tsx*16;
	for(sy=y;sy<y+tsy;sy++) {
	  for(sx=x;sx<x+tsx;sx++) {
	    if ((sx<8)&&(sy<8)) {
	      tx=sy*8+sx;
	      peek[tx]=*look;
	      plot[tx]=1;
	    }
	    look++;
	  }
	  look+=tile->w-tsx;
	}
      }
    }
  }
  idx=y=0;
  while (y<64) {
    look=&peek[idx<<3];
    check=&plot[idx<<3];
    x=0;
    while (x<64) {
      i=*look++;
      if (*check++) {
	if (all) {
	  f=1;
	  SDLmap_plotchr(x,y,i,mode,font);
	} else if ((i&msk)==match) {
	  f=1;
	  ox=i>>rot;
	  if (mode<6) ox=ox<<1;
	  SDLContextBlt(BackContext,x,y,UpdContext,ox*w,0,ox*w+w-1,h-1);
	}
      }
      x+=w;
    }
    y+=h;
    idx++;
  }
  SDLSetContext(MainContext);
  if (f)
    SDLContextBlt(MainContext,96,32,BackContext,0,0,63,63);
  return 1;
}

/*===========================================================================
 * update_font
 * change to a new font bank
 * param b: the new bank to show
 * returns: nothing useful
 *==========================================================================*/
int update_font(int b)
{
  int i,sx,sy;

  font=fontbank[b];
  bank=b;
  SDLSetContext(DialogContext);
  SDLClear(0);
  sx=0; sy=0;
  for(i=0;i<128;i++) {
    SDLplotchr(sx,sy,i,2,font);
    sx+=8;
    if (sx>248) { sx=0; sy+=8; }
  }
  SDLSetContext(MainContext);
  SDLContextBlt(MainContext,8,160,DialogContext,0,0,256,32);
  SDLBox(284,168,292,175,144);
  SDLplotchr(284,168,16+b,2,dfont);
  corner(1);
  return 1;
}

/*===========================================================================
 * draw_edit
 * display the edit screen
 * returns: nothing useful
 *==========================================================================*/
int draw_edit()
{
  int i;

  SDLNoUpdate();
  SDLClear(0);
  SDLstring(192,104,"Char");
  SDLBox(271,159,305,175,144);
  SDLstring(272,160,"Font");
  SDLplotchr(272,168,126,1,dfont);
  SDLplotchr(296,168,127,1,dfont);
  update_font(bank);
  i=echr;
  echr=0;
  grid(i,1);
  panel(1);
  SDLUpdate();
  return 1;
}

/*===========================================================================
 * setup
 * initialize Envision
 * param zoom: pixel zoom level
 * param fullscreen: flag indicating full screen mode
 * returns: nothing useful
 *==========================================================================*/
int setup(int zoom, int fullScreen)
{
  int i;

  dfont=FONT;
  font=fontbank[0];

  for(i=0;i<10;i++) {
    fontbank[i]=(unsigned char *)malloc(1024);
    bank_mod[i]=0;
    memcpy(fontbank[i],dfont,1024);
  }

  if (!initSDL(zoom,fullScreen))
    return 0;

  copy_from=cache=NULL;
  options.disk_image=NULL;
  options.base=options.step=0;
  options.write_tp=1;
  base=bank=0;
  ratio=100;
  mode=2;
  echr=33;
  memset(peek,0,64);
  memset(plot,0,64);

  currentView=map=(view *)malloc(sizeof(view));
  map->w=40; map->h=24;
  map->ch=map->cw=8;
  map->dc=1;
  map->cx=map->cy=map->scx=map->scy=0;
  map->map=(unsigned char *)malloc(map->w*map->h);
  memset(map->map,0,map->w*map->h);
  for(i=0;i<256;i++)
    map->map[i]=i;

  tsx=1; tsy=1;
  tile=(view *)malloc(sizeof(view));
  tile->w=16; tile->h=16;
  tile->dc=1;
  tile->ch=tile->cw=8;
  tile->cx=tile->cy=tile->scx=tile->scy=0;
  tile->map=(unsigned char *)malloc(tile->w*tile->h);
  for(i=0;i<256;i++)
    tile->map[i]=i;

  for(i=0;i<512;i++)
    charTable[i]=NULL;

  cache=(unsigned char *)malloc(40*24);
  cacheOk=0;

  s1=time(NULL);
  s2=0x01234567^s1;

  title();

  setpal();
  draw_edit();
  return 1;
}

/*===========================================================================
 * update
 * update a character, reflecting change on map, etc.
 * param x: grid x-position to change
 * param y: grid y-position to change
 * param c: color (1=on 0=off)
 * returns: nothing useful
 *==========================================================================*/
int update(int x, int y, int c)
{
  unsigned char *dat;

  dat=font+echr*8+y;
  if (c) *dat=*dat|orit[x];
  else *dat=*dat&andit[x];

  topos(echr,&x,&y);
  SDLSetContext(UpdContext);
  SDLBox(0,0,7,7,148);
  SDLplotchr(0,0,echr,2,font);
  SDLSetContext(MainContext);
  SDLContextBlt(MainContext,x,y,UpdContext,0,0,7,7);

  SDLSetContext(UpdContext);
  SDLClear(0);
  SDLplotchr(0,0,echr,6,font);
  SDLplotchr(0,8,echr,7,font);
  SDLplotchr(0,24,echr,4,font);
  SDLSetContext(MainContext);
  SDLContextBlt(MainContext,264,32,UpdContext,0,0,32,56);
  corner(0);
  return 1;
}

/*===========================================================================
 * click
 * handle a click on the edit screen
 * param x: x-position of click
 * param y: y-position of click
 * param b: b is if if left mousebutton is clicked
 * returns: nothing useful
 *==========================================================================*/
int click(int x, int y, int b)
{
  SDL_Event event;
  int i,ox,oy,done;

  if (b!=1)
    b=0;

  if ((cmds[0]==140)&&(x>=22)&&(x<=33)&&(y>=26)&&(y<=31)) {
    panel(2);
    return 0;
  } else if ((cmds[0]==82)&&(x>=9)&&(x<=20)&&(y>=26)&&(y<=31)) {
    panel(1);
    return 0;
  } else if ((x>=272)&&(x<=280)&&(y>=168)&&(y<=176)) {
    bank--;
    if (bank<0) bank=9;
    command(48+bank,0);
    return 0;
  } else if ((x>=296)&&(x<=304)&&(y>=168)&&(y<=176)) {
    bank++;
    if (bank>9) bank=0;
    command(48+bank,0);
    return 0;
  } else if ((x>=8)&&(x<=72)&&(y>=32)&&(y<=cmds[0])) {
    i=(y-22)/10;
    y=(y/10)*10;
    SDLrelease();
    if (copy_from) {
      copy_from=NULL;
      copy_size=0;
      SDLBox(8,152,160,159,0);
    }
    if (cmds[i]) command(cmds[i],0);
    return 0;
  }

  if ((x>=96)&&(x<=159)&&(y>=32)&&(y<=95)) {
    x=(x-96)/8;
    if (mode==3) y=(y-32)/10;
    else y=(y-32)/8;
    if (mode>=6) x=x/2;
    if ((mode==7)||(mode==5)) y=y/2;
    if (plot[(y<<3)+x]) {
      i=peek[(y<<3)+x]&127;
      if (mode>=6) i=(i&63)+base/8;
      grid(i,1);
    }
    return 0;
  }

  ox=x/8; oy=y/8;
  done=0;
  do {
    if ((x>=8)&&(x<264)&&(y>=160)&&(y<192)) {
      x=(x-8)/8; y=(y-160)/8;
      i=y*32+x;
      if (copy_from) {
        if (copy_size) {
          memcpy(undo,font+i*8,8);
          if (i*8+copy_size>1024)
            copy_size=1024-i*8;
          if (copy_size<0) {
            for(y=0;y<8;y++)
              *(font+i*8+y)=*(font+i*8+y)|*(copy_from+y);
          } else memmove(font+i*8,copy_from,copy_size);
          SDLBox(8,152,160,159,0);
          if (copy_size>8) update_font(bank);
          grid(i,0);
          copy_from=NULL;
          copy_size=0;
          return 0;
        } else {
          x=font+i*8-copy_from+8;
          if ((x<0)||(x>1024)) return 0;
          else if (x==0) {
            SDLBox(8,152,160,159,0);
            copy_from=NULL;
            copy_size=0;
            return 0;
          }
          copy_size=x;
          SDLBox(8,152,160,159,0);
          SDLstring(8,152,"Copy range to:");
        }
        return 0;
      }
      grid(i,1);
    } else
      if ((x>=192)&&(x<256)&&(y>=32)&&(y<96)) {
        x=(x/8)*8; y=(y/8)*8;
        if (b)
          SDLBox(x,y,x+7,y+7,10);
        else {
          if ((x/8+y/8)&1) i=144; else i=148;
          SDLBox(x,y,x+7,y+7,i);
        }
        update((x-192)/8,(y-32)/8,b);
      }


    do {
      int err=SDL_WaitEvent(&event);
      if (!err)
        return 0;

      switch (event.type) {
        case SDL_MOUSEBUTTONDOWN: {
          b=(event.button.button==1);
        }
        case SDL_MOUSEBUTTONUP: {
          done=1;
          break;
        }
        case SDL_MOUSEMOTION: {
          x=SDLTranslateClick(event.motion.x)/8;
          y=SDLTranslateClick(event.motion.y)/8;
          break;
        }
        default:
          break;
      }
    } while((x==ox)&&(y==oy)&&(!done));

    ox=x; oy=y;
    x=SDLTranslateClick(event.motion.x);
    y=SDLTranslateClick(event.motion.y);
  } while (!done);
  return 1;
}

/*===========================================================================
 * rotate8x8
 * rotate a character by 90 degrees
 * see GG article
 *==========================================================================*/
void rotate8x8(unsigned char *src, int srcstep, unsigned char *dst, int dststep)
{
  unsigned char *p;
  int pstep, lownyb, hinyb;
  bit32 low, hi;

  low=hi=0;

#define extract(d,t)\
   lownyb = *d & 0xf; hinyb = *d >> 4;\
   low |= t[lownyb]; hi |= t[hinyb]; d += pstep;

  p=src; pstep=srcstep;
  extract(p,ltab0) extract(p,ltab1) extract(p,ltab2) extract(p,ltab3)
  extract(p,ltab4) extract(p,ltab5) extract(p,ltab6) extract(p,ltab7)

#define unpack(d,w)\
   *d=w & 0xff; d+=pstep;\
   *d=(w>>8)&0xff; d+=pstep;\
   *d=(w>>16)&0xff; d+=pstep;\
   *d=(w>>24)&0xff;

  p=dst; pstep=dststep;
  unpack(p,low) p+=pstep; unpack(p,hi)
}

/*===========================================================================
 * left
 * shift a character one step left
 * param dat: character source
 * param work: copy of original data
 * returns: nothing useful
 *==========================================================================*/
int left(unsigned char *dat, unsigned char *work)
{
  int i;

  for(i=0;i<8;i++) {
    dat[i]=dat[i]*2;
    if (work[i]&128) dat[i]+=1;
  }
  return 1;
}

/*===========================================================================
 * right
 * shift a character one step right
 * param dat: character source
 * param work: copy of original data
 * returns: nothing useful
 *==========================================================================*/
int right(unsigned char *dat, unsigned char *work)
{
  int i;

  for(i=0;i<8;i++) {
    dat[i]=dat[i]/2;
    if (work[i]&1) dat[i]+=128;
  }
  return 1;
}

/*===========================================================================
 * right
 * shift a character one step up
 * param dat: character source
 * param work: copy of original data
 * returns: nothing useful
 *==========================================================================*/
int up(unsigned char *dat, unsigned char *work)
{
  int i;
  for(i=1;i<8;i++)
    dat[i-1]=work[i];
  dat[7]=work[0];
  return 1;
}

/*===========================================================================
 * down
 * shift a character one step down
 * param dat: character source
 * param work: copy of original data
 * returns: nothing useful
 *==========================================================================*/
int down(unsigned char *dat, unsigned char *work)
{
  int i;
  for(i=1;i<8;i++)
    dat[i]=work[i-1];
  dat[0]=work[7];
  return 1;
}

/*===========================================================================
 * command
 * handle an editor command
 * param cmd: the command to process
 * param sym: extended information
 * returns: 0 if exiting program
 *==========================================================================*/
int command(int cmd, int sym)
{
  unsigned char *dat, l, work[8];
  int i,j;
  char buf[4], *fname;

  dat=font+echr*8;
  memcpy(work,dat,8);

  if (sym) {
    switch(sym) {
      case SDLK_ESCAPE: {
        bye();
        break;
      }
      case SDLK_LEFT: {
        left(dat,work);
        break;
      }
      case SDLK_RIGHT: {
        right(dat,work);
        break;
      }
      case SDLK_UP: {
        up(dat,work);
        break;
      }
      case SDLK_DOWN: {
        down(dat,work);
        break;
      }
    }
  }

  switch (cmd) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      update_font(cmd-'0');
      break;
    }
    case 'b': {
      memset(dat,0,8);
      break;
    }
    case 'i': {
      for(i=0;i<8;i++)
        *(dat+i)=255-*(dat+i);
      break;
    }
    case 'a': {
      memcpy(dat,dfont+echr*8,8);
      break;
    }
    case 'v': {
      for(i=0;i<8;i++)
        *(dat+i)=work[7-i];
      break;
    }
    case 'r': {
      rotate8x8(work,1,dat,1);
      break;
    }
    case 'u': {
      memcpy(dat,undo,8);
      memcpy(undo,work,8);
      break;
    }
    case 'h': {
      for(i=0;i<8;i++) {
        l=work[i];
        *(dat+i)=0;
        for(j=0;j<8;j++)
          if (l&orit[j]) *(dat+i)+=orit[7-j];
      }
      break;
    }
    case 'c': {
      copy_from=dat;
      copy_size=8;
      SDLstring(8,152,"Copy to:");
      break;
    }
    case 'x': {
      copy_from=dat;
      copy_size=0;
      SDLstring(8,152,"Select range end:");
      break;
    }
    case 't': {
      copy_from=dat;
      copy_size=-8;
      SDLstring(8,152,"Transcopy to:");
      break;
    }
    case 'A': {
      memcpy(font,dfont,1024);
      update_font(bank);
      break;
    }
    case 'n': {
      values++;
      if (values==3) values=0;
      SDLBox(168,32,191,96,0);
      if (values) {
        for(i=0;i<8;i++) {
          if (values==2)
            sprintf(buf," %02x",work[i]);
          else
            sprintf(buf,"%3d",work[i]);
          SDLstring(168,32+i*8,buf);
        }
      }
      break;
    }
    case 's': {
      fname=get_filename("Save font:",options.disk_image);
      if (fname) {
        if (options.disk_image)
          i=write_xfd_font(options.disk_image,fname,font,1024,NULL);
        else
          i=write_font(fname,font);
        free(fname);
      }
      break;
    }
    case 'l': {
      fname=get_filename("Load font:",options.disk_image);
      if (fname) {
        if (options.disk_image)
          i=read_xfd_font(options.disk_image,fname,font,1024);
        else
          i=read_font(fname,font);
        free(fname);
        update_font(bank);
      }
      break;
    }
    case 'e': {
      fname=get_filename("Export font:",options.disk_image);
      if (fname) {
        if (options.disk_image)
          i=write_xfd_data(options.disk_image,fname,font,0,127);
        else
          i=write_data(fname,font,0,127,0);
        free(fname);
      }
      break;
    }
    case 'o': {
      do_options();
      break;
    }
    case 'p': {
      do_colors();
      corner(1);
      break;
    }
    case 'm': {
      do_map();
      draw_edit();
      return 0;
    }
    default: {
      /*  printf("%d ",cmd); */
      if (!sym)
        return 0;
    }
  }
  i=echr; echr=0; grid(i,0);
  return 0;
}

/*===========================================================================
 * edit
 * the event loop for the editor
 * returns: nothing useful
 *==========================================================================*/
int edit()
{
  SDL_Event event;
  int done=0;

  do {
    SDL_WaitEvent(&event);

    switch(event.type){  /* Process the appropiate event type */
      case SDL_QUIT: {
        command(0,SDLK_ESCAPE);
        break;
      }
      case SDL_KEYDOWN: {
        int sym=event.key.keysym.sym;
        int ch=event.key.keysym.unicode&0x7f;

        if ((sym=='q')&&(event.key.keysym.mod&KMOD_CTRL)) {
          sym=SDLK_ESCAPE;  /* handle ctrl-q */
        }
        if ((sym==SDLK_ESCAPE)||(sym==SDLK_LEFT)||(sym==SDLK_RIGHT)||
            (sym==SDLK_UP)||(sym==SDLK_DOWN)) {
          command(ch,sym);
        } else {
          command(ch,0);
        }
        break;
      }
      case SDL_MOUSEBUTTONDOWN: {
        int mx,my;
        mx=SDLTranslateClick(event.button.x);
        my=SDLTranslateClick(event.button.y);
        click(mx,my,(event.button.button==1));
        break;
      }
      default:
        break;
    }
  } while(!done);
  return 0;
}

/*===========================================================================
 * main
 * start here...
 * params: the usual
 * returns: nothing useful
 *==========================================================================*/
int main(int argc, char *argv[])
{
  int i,zoom, fullScreen;

  zoom=2;
  fullScreen=0;

  for(i=1;i<argc;i++) {
    if ((!strcmp(argv[i],"-z"))||(!strcmp(argv[i],"-zoom"))) {
      if (i<argc-1) {
        i++;
        zoom=atoi(argv[i]);
        if (zoom<1)
          zoom=3;
        if (zoom>8)
          zoom=8;
      }
    } else if ((!strcmp(argv[i],"-f"))||(!strcmp(argv[i],"-full"))) {
      fullScreen=1;
    }
  }
  setup(zoom,fullScreen);
  edit();
  return 0;
}
/*=========================================================================*/
