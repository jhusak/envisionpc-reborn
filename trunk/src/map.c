/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: map.c                                                            *
 * Author: Mark Schmelzenbach                                             *
 * Started: 07/30/97                                                      *
 * Long overdue port to SDL 1/26/2006                                     *
 =========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDLemu.h"
#include "envision.h"

view *currentView, *map, *tile; /* display, map, tile */
int tm; /* typing mode flag */
int mode, hidden, ratio; /* ANTIC mode, menu shown flag, replace ratio */
int base; /* character base */
int tsx, tsy; /* tile width, height */
int cacheOk, tmode; /* cache valid flag, tile mode flag */
unsigned char *cache; /* cache */
SDL_Surface *charTable[512];

/*===========================================================================
 * Based on George Marsaglia's uniform random number generator
 * (has a very large period, > 2^60, and passes Diehard tests;
 * uses a multiply-with-carry method)
 *==========================================================================*/
#define s1new  ((s1=36969*(s1&65535)+(s1>>16))<<16)
#define s2new  ((s2=18000*(s2&65535)+(s2>>16))&65535)
#define UNI    ((s1new+s2new)*2.32830643708e-10)
unsigned long s1, s2;

/*===========================================================================
 * draw_cursor
 * draw the cursor to the screen using XOR write
 * returns: nothing useful
 *==========================================================================*/
int draw_cursor() {
	if (hidden) SDLClip(2);
	else SDLClip(1);

	SDLXORBox(currentView->cx*currentView->cw,24+currentView->cy*currentView->ch,currentView->cx*currentView->cw+currentView->cw-1,24+currentView->cy*currentView->ch+currentView->ch-1);
	if ((tmode)&&((tsx>1)||(tsy>1))) {
		int tx, ty;
		tx=(((currentView->cx+(currentView->scx%tsx))/tsx)*tsx)-currentView->scx%tsx;
		ty=(((currentView->cy+(currentView->scy%tsy))/tsy)*tsy)-currentView->scy%tsy;
		SDLXORHollowBox(tx*currentView->cw,24+ty*currentView->ch,tx*currentView->cw+currentView->cw*tsx-1,24+ty*currentView->ch+currentView->ch*tsy-1);
	}
	SDLClip(0);
	return 1;
}

/*===========================================================================
 * set_mapview
 * ensure display is showing a specific view;
 * param id: the view to display (0=map; 1=tile)
 * returns: nothing useful
 *==========================================================================*/
int set_mapview(int id) {
	view *old=currentView;
	if (!id) {
		tmode=0;
		do_mode(mode); /* update tiles, if necessary */
		currentView=map;
	} else {
		tmode=1;
		currentView=tile;
	}
	if (currentView!=old)
		cacheOk=0;
	return 1;
}

/*===========================================================================
 * tile_size
 * change the tile size
 * param w: number of characters wide a tile is
 * param h: number of characters high a tile is
 *==========================================================================*/
int tile_size(int w, int h) {
	int i;

	if ((tsx==w)&&(tsy==h))
		return 0;

	free(tile->map);
	tile->map=(unsigned char *)malloc(w*h*256);
	tile->cx=tile->cy=tile->scx=tile->scy=0;
	tile->w=w*16; tile->h=h*16;
	tsx=w; tsy=h;

	if ((w==1)&&(h==1)) {  /* special default 1x1 case */
		for(i=0;i<256;i++)
			tile->map[i]=i;
	} else {
		memset(tile->map,0,w*h*256);
	}
	return 1;
}

/*===========================================================================
 * map_panel
 * render the map command panel to UpdContext
 * returns: nothing useful
 *==========================================================================*/
int map_panel()
{
	SDLSetContext(UpdContext);
	SDLClear(0);
	drawbutton(0,0,"Resi*Ze"); cmds[1]='z';
	drawbutton(0,10,"*Mode"); cmds[2]='m';
	drawbutton(0,20,"*Ratio"); cmds[3]='r';
	drawbutton(0,30,"*Find"); cmds[4]='f';
	drawbutton(0,40,"*Draw ch"); cmds[5]='d';
	drawbutton(0,50,"*Save"); cmds[6]='s';
	drawbutton(0,60,"*Load"); cmds[7]='l';
	drawbutton(0,70,"*Clear"); cmds[8]='c';
	drawbutton(0,80,"*Block"); cmds[9]='b';
	drawbutton(0,90,"*Go to"); cmds[10]='g';
	drawbutton(0,100,"Ret*Ile"); cmds[11]='i';
	drawbutton(0,110,"*Hide"); cmds[12]='h';
	cmds[0]=143;
	SDLSetContext(MainContext);
	return 1;
}

/*===========================================================================
 * curs_pos
 * update the status bar with appropriate cursor information
 * returns: nothing useful
 *==========================================================================*/
int curs_pos()
{
	char buf[16];
	int back;

	SDLSetContext(DialogContext);
	if (!tmode) {
		if ((currentView->w>999)||(currentView->h>999)) {
			sprintf(buf,"X:%05d Y:%05d",currentView->scx+currentView->cx,currentView->scy+currentView->cy);
			back=32;
		} else {
			sprintf(buf,"X:%03d Y:%03d",currentView->scx+currentView->cx,currentView->scy+currentView->cy);
			back=0;
		}
	} else {
		int tx, ty;
		tx=((currentView->scx+currentView->cx)/tsx);
		ty=((currentView->scy+currentView->cy)/tsy);
		sprintf(buf,"Tile: %03d",tx+ty*16);
		back=0;
	}
	SDLBox(0,0,118,7,144);
	SDLstring(0,0,buf);
	SDLSetContext(MainContext);
	SDLContextBlt(MainContext,228-back,6,DialogContext,0,0,118,7);
	return 1;
}

/*===========================================================================
 * move
 * move the cursor by a given distance
 * param dx: horizontal delta
 * param dy: vertical delta
 * returns: nothing useful
 *==========================================================================*/
int move(int dx, int dy)
{
	int ox,oy;

	SDLNoUpdate();
	if ((currentView->cx+currentView->scx+dx>=currentView->w)||
			(currentView->cy+currentView->scy+dy>=currentView->h))
		return 0;

	if ((currentView->cx+currentView->scx+dx<0)||
			(currentView->cy+currentView->scy+dy<0))
		return 0;

	draw_cursor();

	ox=currentView->cx; oy=currentView->cy;
	currentView->cx+=dx; currentView->cy+=dy;

	if (currentView->cx<0) {
		currentView->cx=0;
		if (currentView->scx) currentView->scx--;
		else return draw_cursor();
	} else if (currentView->cx*currentView->cw>320-currentView->cw) {
		currentView->cx--;
		if (currentView->cx+currentView->scx+1<currentView->w) currentView->scx++;
		else return draw_cursor();
	}
	if (currentView->cy<0) {
		currentView->cy=0;
		if (currentView->scy) currentView->scy--;
		else return draw_cursor();
	} else if (currentView->cy*currentView->ch+24>200-currentView->ch) {
		currentView->cy--;
		if (currentView->cy+currentView->scy>=currentView->h-1)
			return draw_cursor();
		else currentView->scy++;
	}
	if (tm) {
		SDLBox(308,15,316,22,144);
		m=get_8x8_mode(mode);
		SDLmap_plotchr(308,15,currentView->map[(currentView->cx+currentView->scx)+(currentView->cy+currentView->scy)*currentView->w],m,font);
	}
	curs_pos();
	if ((ox==currentView->cx)&&(oy==currentView->cy)) draw_screen(1);
	else draw_cursor();
	return 1;
}

/*===========================================================================
 * map_command
 * process a command
 * param cmd: the command
 * param sym: extended command information
 * returns: 0 if user is exiting map screen
 *==========================================================================*/
int map_command(int cmd, int sym)
{
	int i,j,r,f;
	char *fname;
	unsigned char *look;

	if (tm) {
		if (!sym) {
			currentView->map[(currentView->cx+currentView->scx)+(currentView->cy+currentView->scy)*currentView->w]=cmd;
			draw_cursor();
			SDLCharBlt(MainContext,currentView->cx*currentView->cw,24+currentView->cy*currentView->ch,cmd);
			draw_cursor();
			move(1,0);
			cmd=0;
		}
	}

	if (sym) {
		switch(sym) {
			case SDLK_ESCAPE: {
						  if (!tm)
							  bye();
						  else {
							  tm=0;
							  draw_header(1);
							  return 0;
						  }
						  break;
					  }
			case SDLK_LEFT: {
						move(-1,0);
						return 0;
						break;
					}
			case SDLK_RIGHT: {
						 move(1,0);
						 return 0;
						 break;
					 }
			case SDLK_UP: {
					      move(0,-1);
					      return 0;
					      break;
				      }
			case SDLK_DOWN: {
						move(0,1);
						return 0;
						break;
					}
		}
	}

	switch (cmd) {
		case 'h': {
				  hidden=!hidden;
				  cacheOk=0;
				  break;
			  }
		case 'f': {
				  if ((!tmode)&&((tsx>1)||(tsy>1)))
					  f=currentView->dc=get_number("Tile to replace:", 1,255);
				  else
					  f=select_draw("Select character to replace:");
				  j=currentView->w*currentView->h;
				  look=currentView->map;
				  for(i=0;i<j;i++) {
					  if (*look==f) {
						  r=(int)(100.0*UNI);
						  if (r<ratio) *look=currentView->dc;
					  }
					  look++;
				  }
				  cacheOk=0;
				  draw_header(0);
				  break;
			  }
		case 'c': {
				  if (ratio==100) {
					  memset(currentView->map,currentView->dc,currentView->w*currentView->h);
				  } else {
					  j=currentView->w*currentView->h;
					  look=currentView->map;
					  for(i=0;i<j;i++) {
						  r=(int)(100.0*UNI);
						  if (r<ratio) *look=currentView->dc;
						  look++;
					  }
				  }
				  cacheOk=0;
				  break;
			  }
		case 'r': {
				  ratio=get_number("Enter ratio:",ratio,100);
				  return 0;
			  }
		case 'd': {
				  if ((!tmode)&&((tsx>1)||(tsy>1)))
					  currentView->dc=get_number("New draw tile:", currentView->dc,255);
				  else
					  currentView->dc=select_draw("Select or type new draw char:");
				  draw_header(0);
				  cacheOk=0;
				  break;
			  }
		case 'm': {
				  do {
					  i=get_number("Enter ANTIC mode:",mode,-1);
				  } while ((i>=0)&&((i<2)||(i>7)));
				  if (mode==i) return 0;
				  if (i>0) {
					  currentView->cx=currentView->cy=currentView->scx=currentView->scy=0;
					  mode=i;
					  draw_header(0);
					  do_mode(mode);
				  }
				  break;
			  }
		case 'z': {
				  do_size(tmode);
				  draw_header(0);
				  cacheOk=0;
				  break;
			  }
		case 'i': {
				  int itsx, itsy;
				  if (tmode)
					  break;

				  cacheOk=0;
				  itsx=itsy=0;
				  if ((tsy==1)&&(tsx==1)) {
					  do {
						  itsx=get_number("Enter tile width:",2,8);
					  } while((itsx<1)||(itsx>8)||(itsx>map->w));
					  do {
						  itsy=get_number("Enter tile height:",2,8);	
					  } while((itsy<1)||(itsy>8)||(itsy>map->h));
					  if ((itsy==1)&&(itsx==1))
						  break;
				  }      
				  set_mapview(1);
				  if ((tsy>1)||(tsx>1)) {
					  untile_map(map,tile);
				  } else {
					  tile_map(itsx,itsy,map,tile);
				  }
				  set_mapview(0);
				  draw_header(0);
				  break;
			  }
		case 'u': {
				  if (base) base=0;
				  else base=64*8;
				  do_mode(mode);
				  break;
			  }
		case 'e': {
				  return 1;
			  }
		case 's': {
				  fname=get_filename("Save map:",options.disk_image);
				  if (fname) {
					  if (options.disk_image)
						  i=write_xfd_map(options.disk_image,fname,font,map,0);
					  else
						  i=write_map(fname,font,map,0);
					  free(fname);
				  }
				  return 0;
				  break;
			  }
		case 'w': {
				  fname=get_filename("Save raw map:",options.disk_image);
				  if (fname) {
					  if (options.disk_image)
						  i=write_xfd_map(options.disk_image,fname,font,map,1);
					  else
						  i=write_map(fname,font,map,1);
					  free(fname);
				  }
				  return 0;
				  break;
			  }
		case 'l': {
				  fname=get_filename("Load map:",options.disk_image);
				  if (fname) {
					  if (options.disk_image)
						  read_xfd_map(options.disk_image,fname,font,map);
					  else
						  read_map(fname,font,map);
					  free(fname);
					  draw_header(0);
					  do_mode(mode);
				  }
				  break;
			  }
		case 't': {
				  if ((!tmode)&&((tsx>1)||(tsy>1)))
					  break;
				  tm=1;
				  SDLBox(228,15,319,22,144);
				  SDLstring(228,15,"Type Mode");
				  cacheOk=0;
				  hidden=1;
				  break;
			  }
		case 'g': {
				  do_move(currentView,tmode);
				  cacheOk=0;
				  curs_pos();
				  break;
			  }
		case 'b': {
				  set_mapview(!tmode);
				  if (!cacheOk)
					  draw_header(0);
				  break;
			  }
		default: {
				 return 0;
			 }
	}
	draw_screen(1);
	return 0;
}

/*===========================================================================
 * map_click
 * process a mouse click
 * param x: x position of the mouse
 * param y: y position of the mouse
 * param b: 1 if left mousebutton is pressed
 * param down: returns mouse button status
 * returns: 0 if user is exiting map screen
 *==========================================================================*/
int map_click(int x, int y, int b, int *down)
{
	int i,ox,oy;

	if ((!hidden)&&(x>=251)&&(x<=315)&&(y>=24)&&(y<=cmds[0])) {
		i=(y-14)/10;
		SDLrelease(); /* wait for mouse to unclick */
		*down=0;
		if (cmds[i]) return(map_command(cmds[i],0));
		return 0;
	}
	if (y>=24) {
		oy=i=(y-24)/currentView->ch;
		ox=x=x/currentView->cw;
		if ((ox+currentView->scx>=currentView->w)||
				(i+currentView->scy>=currentView->h))
			return 0;
		if ((b)&&(!tm)) {
			currentView->map[(x+currentView->scx)+(i+currentView->scy)*currentView->w]=currentView->dc;
			x=x*currentView->cw;
			y=24+i*currentView->ch;
			if ((!tmode)&&((tsx!=1)||(tsy!=1))) {
				SDLCharBlt(MainContext,x,y,currentView->dc+256);
			} else {
				SDLCharBlt(MainContext,x,y,currentView->dc);
			}
			if ((!hidden)&&(x+currentView->cw>250))
				draw_screen(b);
		}
		move(ox-currentView->cx,oy-currentView->cy);
	}
	return 0;
}

/*===========================================================================
 * draw_screen
 * redraw the screen view
 * param b: 1 if left mousebutton is pressed
 * returns: nothing useful
 *==========================================================================*/
int draw_screen(int b)
{
	int i,w,h;
	int x,y,ofs,lx,ly,by,bx;
	unsigned char *max, *look, *check;

	SDLNoUpdate();
	look=currentView->map+currentView->scy*currentView->w+currentView->scx;
	max=currentView->map+currentView->w*currentView->h;
	lx=currentView->cx*currentView->cw;
	ly=currentView->cy*currentView->ch;

	w=currentView->cw*currentView->w;
	h=currentView->ch*currentView->h;
	if (w>320)
		w=320;
	if (h>176)
		h=176;

	ofs=currentView->w-((w+currentView->cw-1)/currentView->cw);
	by=y=0;
	check=cache;
	SDLClip(!hidden);

	while ((y<h)&&(!by)) {
		x=bx=0;
		while (x<w) {
			bx++;
			if (currentView->scx+bx>currentView->w) {
				SDLBox(x,24+y,320,24+y+currentView->ch,144);
				look++;
				break;
			}
			if (look<max)
				i=*look++;
			else {
				by=y; break;
			} 
			if ((!tmode)&&((tsx!=1)||(tsy!=1)))
				i+=256;
			if ((!cacheOk)||(*check!=i)) {
				SDLCharBlt(MainContext,x,24+y,i);
			}
			x+=currentView->cw;
			*check++=i;
		}
		look=look+ofs;
		y+=currentView->ch;
	}
	cacheOk=1;
	if (by) {
		SDLBox(0,by+24,320,200,144);
		SDLBox(0,by+24,320,by+24,0);
		cacheOk=0; /* should be able to avoid this by drawing last line */
	}
	if ((w<320)||(h<176)) {
		SDLBox(w,24,320,200,144);
		SDLBox(0,h+24,320,200,144);
	} 
	SDLClip(0);
	if (!hidden) {
		SDLBox(248,24,319,200,0);
		SDLContextBlt(MainContext,251,24,UpdContext,0,0,64,119);
	}
	draw_cursor();
	//  SDLUpdate();
	return 1;
}

/*===========================================================================
 * do_mode
 * change the ANTIC display mode, updating characters and tiles
 * param m: ANTIC mode
 * returns: nothing useful
 *==========================================================================*/
int do_mode(int m)
{
	int i,w,h;
	unsigned char *look;

	cacheOk=0;
	if ((m==2)||(m==4)) {
		tile->ch=tile->cw=8;
	} else if (m==3) {
		tile->cw=8; tile->ch=10;
	} else if (m==5) {
		tile->cw=8; tile->ch=16;
	} else if (m==6) {
		tile->cw=16; tile->ch=8;
	} else if (m==7) {
		tile->ch=tile->cw=16;
	}
	map->cw=tile->cw*tsx; map->ch=tile->ch*tsy;
	/* build characters... */
	for(i=0;i<256;i++) {
		SDLRebuildChar(i,tile->cw,tile->ch);
		SDLClear(clut[0]);
		SDLmap_plotchr(0,0,i,m,font);
	}
	/* build tiles... */
	for(i=0;i<256;i++) {
		if ((tsx!=1)||(tsy!=1)) {
			int tx,ty;
			ty=i/16;
			tx=i-ty*16;
			look=tile->map+tx*tsx+ty*tsy*tsx*16;
			SDLRebuildChar(256+i,tsx*tile->cw,tsy*tile->ch);
			SDLClear(clut[0]);
			for(h=0;h<tsy;h++) {
				for(w=0;w<tsx;w++) {
					SDLmap_plotchr(w*tile->cw,h*tile->ch,*look,m,font);
					look++;
				}
				look+=tile->w-tsx;
			}
		} else {
			SDLRebuildChar(256+i,0,0);
		}
	}
	SDLSetContext(MainContext);

	return 1;
}

/*===========================================================================
 * draw_header
 * update map mode header
 * param update: if true, refresh the screen
 * returns: nothing useful
 *==========================================================================*/
int draw_header(int update)
{
	char buf[40];
	int m;
	m=get_8x8_mode(mode);
	SDLNoUpdate();
	SDLBox(0,6,319,22,144);
	SDLHLine(0,319,14,0);
	if (!tmode) {
		SDLstring(8,6,"Map Editor - ");
		if ((currentView->w>999)||(currentView->h>999))
			sprintf(buf,"Mode %02d [%05dx%05d]",mode,currentView->w,currentView->h);
		else
			sprintf(buf,"Mode %02d [%03dx%03d]",mode,currentView->w,currentView->h);
	} else {
		SDLstring(8,6,"Tile Editor - ");
		sprintf(buf,"Size [%02dx%02d]",tsx,tsy);
	}
	SDLstring(8,15,buf);
	if ((!tmode)&&((tsx>1)||(tsy>1))) {
		sprintf(buf,"Tile: %03d",currentView->dc);
		SDLstring(228,15,buf);
	} else {
		SDLstring(228,15,"Draw Char:");

		SDLmap_plotchr(308,15,currentView->dc,m,font);
	}
	curs_pos();
	return 1;
}

/*===========================================================================
 * do_map
 * Event look for map and tile modes
 * returns: nothing useful
 *==========================================================================*/
int do_map()
{
	SDL_Event event;
	int down,done;

	down=done=0;
	set_mapview(0);

	SDLClear(0);
	draw_header(0);
	do_mode(mode);
	map_panel();
	draw_screen(1);
	cacheOk=0;
	tm=0;

	do {
		SDLUpdate();
		SDL_WaitEvent(&event);

		switch(event.type){  /* Process the appropiate event type */
			case SDL_QUIT: {
					       done=map_command(0,SDLK_ESCAPE);
				       }
			case SDL_KEYDOWN: {
						  int sym=event.key.keysym.sym;
						  int ch=event.key.keysym.unicode&0x7f;

						  if ((sym=='q')&&(event.key.keysym.mod&KMOD_CTRL)) {
							  sym=SDLK_ESCAPE;  /* handle ctrl-q */
						  }
						  if ((sym==SDLK_ESCAPE)||(sym==SDLK_LEFT)||(sym==SDLK_RIGHT)||
								  (sym==SDLK_UP)||(sym==SDLK_DOWN)) {
							  done=map_command(ch,sym);
						  } else if (ch) {
							  if (tm) {
								  ch=stoa(ch);
								  if ((event.key.keysym.mod&KMOD_ALT))
									  ch+=128;
							  }
							  done=map_command(ch,0);
						  }
						  break;
					  }
			case SDL_MOUSEBUTTONDOWN: {
							  int mx,my;
							  mx=SDLTranslateClick(event.button.x);
							  my=SDLTranslateClick(event.button.y);
							  down=event.button.button;
							  if (down==3)
								  SDLUpdate();
							  done=map_click(mx,my,event.button.button==1,&down);
							  break;
						  }
			case SDL_MOUSEMOTION: {
						      if (down) {
							      int mx,my,ox,oy;
							      mx=SDLTranslateClick(event.button.x);
							      my=SDLTranslateClick(event.button.y);
							      oy=(my-24)/currentView->ch;
							      ox=mx/currentView->cw;
							      if (((hidden)||(mx<250))&&((ox!=currentView->cx)||(oy!=currentView->cy))) {
								      done=map_click(mx,my,down==1,&down);
							      }
						      }
						      break;
					      }
			case SDL_MOUSEBUTTONUP: {
							down=0;
							break;
						}
			default:
						break;
		}
	} while(!done);
	set_mapview(0);
	return 0;
}
/*=========================================================================*/
