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

view *currentView, *map, *tile, *mask, *undomap; /* display, map, tile, mask, undobufferformapandmask */
int typeMode; /* typing mode flag */
int mode, hidden, ratio; /* ANTIC mode, menu shown flag, replace ratio */
int base; /* character base */
int tsx, tsy; /* tile width, height */
int cacheOk, tileEditMode; /* cache valid flag, tile edit mode flag */
int maskEditMode; /* mask mode edit flag */
int do_probe; /* probe flag; if set, the char/tile the user clicked goes to draw_char */
unsigned char *cache; /* cache */
SDL_Surface *charTable[512];

int MAP_MENU_HEIGHT;

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
	//if NOT_MASK_EDIT_MODE
		SDLXORBox(currentView->cx*currentView->cw,MAP_TOP_OFFSET+currentView->cy*currentView->ch,currentView->cx*currentView->cw+currentView->cw-1,MAP_TOP_OFFSET+currentView->cy*currentView->ch+currentView->ch-1);
	//else
	//	SDLXORDottedBox(currentView->cx*currentView->cw,MAP_TOP_OFFSET+currentView->cy*currentView->ch,currentView->cx*currentView->cw+currentView->cw-1,MAP_TOP_OFFSET+currentView->cy*currentView->ch+currentView->ch-1);
	// draw hollow box
	if ((tileEditMode) &&((tsx>1)||(tsy>1))) {
		int tx, ty;
		tx=(((currentView->cx+(currentView->scx%tsx))/tsx)*tsx)-currentView->scx%tsx;
		ty=(((currentView->cy+(currentView->scy%tsy))/tsy)*tsy)-currentView->scy%tsy;
		SDLXORHollowBox(tx*currentView->cw,MAP_TOP_OFFSET+ty*currentView->ch,tx*currentView->cw+currentView->cw*tsx-1,MAP_TOP_OFFSET+ty*currentView->ch+currentView->ch*tsy-1);


	}
	SDLClip(0);
	return 1;
}


/*===========================================================================
 * set_mapview
 * ensure display is showing a specific view;
 * param id: the view to display (0=map; 1=tile, 2=mask)
 * returns: nothing useful
 *==========================================================================*/
int set_mapview(int id) {
	view *old=currentView;
	switch (id) {
		case VIEW_MAP:
			tileEditMode=0;
			do_mode(mode); /* update tiles, if necessary */
			currentView=map;
			break;
		case VIEW_TILE: 
			tileEditMode=1;
			currentView=tile;
			break;
		case VIEW_MASK:
			tileEditMode=0;
			currentView=mask;
			break;
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
	
	int cmdcnt=0;
	if NOT_MASK_EDIT_MODE {
		drawbutton_map(0,cmdcnt*10,"Go to *edit");	cmds[++cmdcnt]='e';
		//drawbutton(0,cmdcnt*10,"*undo");	cmds[++cmdcnt]='u';
		drawbutton_map(0,cmdcnt*10,"Resi*ze Map");	cmds[++cmdcnt]='z';
		drawbutton_map(0,cmdcnt*10,"Antic *mode");	cmds[++cmdcnt]='m';
		drawbutton_map(0,cmdcnt*10,"Shift CHBase *Up");	cmds[++cmdcnt]='U';
		drawbutton_map(0,cmdcnt*10,"*ratio");	cmds[++cmdcnt]='r';
		drawbutton_map(0,cmdcnt*10,"*find");	cmds[++cmdcnt]='f';
		drawbutton_map(0,cmdcnt*10,"*draw char"); cmds[++cmdcnt]='d';
		drawbutton_map(0,cmdcnt*10,"*probe"); cmds[++cmdcnt]='p';
		drawbutton_map(0,cmdcnt*10,"*load map");	cmds[++cmdcnt]='l';
		drawbutton_map(0,cmdcnt*10,"*Read raw map");	cmds[++cmdcnt]='R';
		drawbutton_map(0,cmdcnt*10,"*save map");	cmds[++cmdcnt]='s';
		drawbutton_map(0,cmdcnt*10,"*write raw map");	cmds[++cmdcnt]='w';
		drawbutton_map(0,cmdcnt*10,"*clear map");	cmds[++cmdcnt]='c';
		drawbutton_map(0,cmdcnt*10,"*block");	cmds[++cmdcnt]='b';
		drawbutton_map(0,cmdcnt*10,"ret*ile");	cmds[++cmdcnt]='i';
		if NOT_TILE_MODE {drawbutton_map(0,cmdcnt*10,"*type mode");	cmds[++cmdcnt]='t';	}
	} else {
		drawbutton_map(0,cmdcnt*10,"exit M*ask Edit");	cmds[++cmdcnt]='a';
		//drawbutton_map(0,cmdcnt*10,"*undo");	cmds[++cmdcnt]='u';
		drawbutton_map(0,cmdcnt*10,"Resi*ze Map");	cmds[++cmdcnt]='z';
		drawbutton_map(0,cmdcnt*10,"Shift CHBase *Up");	cmds[++cmdcnt]='U';
		drawbutton_map(0,cmdcnt*10,"*Read Raw Mask");	cmds[++cmdcnt]='R';
		drawbutton_map(0,cmdcnt*10,"*write Raw Mask");	cmds[++cmdcnt]='w';
		drawbutton_map(0,cmdcnt*10,"*set mask");	cmds[++cmdcnt]='s';
		drawbutton_map(0,cmdcnt*10,"*clear mask");	cmds[++cmdcnt]='c';
		drawbutton_map(0,cmdcnt*10,"set from *map");	cmds[++cmdcnt]='m';
	}
	
	drawbutton_map(0,cmdcnt*10,"*go to XY");	cmds[++cmdcnt]='g';
	if (NOT_MASK_EDIT_MODE && !tileEditMode)  { drawbutton_map(0,cmdcnt*10,"M*ask edit");	cmds[++cmdcnt]='a';}
		drawbutton_map(0,cmdcnt*10,"*hide");	cmds[++cmdcnt]='h';
	// wolne:ajknoqvxy
	MAP_MENU_HEIGHT=cmdcnt*10;
	
	set_allowed_commands(cmds,cmdcnt,1);
	
	cmds[0]=MAP_MENU_HEIGHT*10+10;
	
	if NOT_MASK_EDIT_MODE {
		select_draw(0,CONFIG.screenHeight-171,"Char chooser:", map->dc, 16,0);
	}
	
	SDLSetContext(MainContext);
	return 1;
}


void update_map_font(int b)
{
	font=fontbank[b];
	bank=b;
	if (mode!=fontmode[b]) {
		mode=fontmode[b];
		do_mode(mode);
	}
}


/*===========================================================================
 * curs_pos
 * update the status bar with appropriate cursor information
 * returns: nothing useful
 *==========================================================================*/
int curs_pos()
{
	char buf[32];
	int back;

	SDLSetContext(DialogContext);
	if (!tileEditMode) {
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
	SDLContextBlt(MainContext,CONFIG.screenWidth-92-back,6,DialogContext,0,0,118,7);
	/* DEBUG PURPOSES JH
	SDLBox(28*8,6,48*8,22,144);
	sprintf(buf,"cxcy  [%05dx%05d]",currentView->cx,currentView->cy);
	SDLstring(28*8,6,buf);
	sprintf(buf,"scxscy[%05dx%05d]",currentView->scx,currentView->scy);
	SDLstring(28*8,14,buf);
	 */
	
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
	int ox,oy,m;

	if ((currentView->cx+currentView->scx+dx>=currentView->w)||
			(currentView->cy+currentView->scy+dy>=currentView->h))
		return 0;

	if ((currentView->cx+currentView->scx+dx<0)||
			(currentView->cy+currentView->scy+dy<0))
		return 0;

	// this tears-off the cursor previously set
	draw_cursor();

	// change cursor coords
	ox=currentView->cx; oy=currentView->cy;
	currentView->cx+=dx; currentView->cy+=dy;

	// clip them and when they are outside move the window if not out of bounds
	if (currentView->cx<0) {
		currentView->cx=0;
		if (currentView->scx) currentView->scx--;
		else return draw_cursor();
	} else if (currentView->cx*currentView->cw>CONFIG.screenWidth-currentView->cw) {
		currentView->cx--;
		if (currentView->cx+currentView->scx+1<currentView->w)
		{  
			currentView->scx++;
			// rather wrong way of bugfix for shadows in type-mode when right side of the screen reached
			//and right side of the map not yet.
			cacheOk=0;
		}
		else return draw_cursor();
	}
	
	if (currentView->cy<0) {
		currentView->cy=0;
		if (currentView->scy) currentView->scy--;
		else return draw_cursor();
	} else if (currentView->cy*currentView->ch+MAP_TOP_OFFSET>CONFIG.screenHeight-currentView->ch) {
		currentView->cy--;
		if (currentView->cy+currentView->scy>=currentView->h-1)
			return draw_cursor();
		else currentView->scy++;
	}
	
	if (typeMode) {
		SDLBox(CONFIG.screenWidth-12,15,CONFIG.screenWidth-4,22,144);
		m=get_8x8_mode(mode);
		SDLmap_plotchr(CONFIG.screenWidth-12,15,currentView->map[(currentView->cx+currentView->scx)+(currentView->cy+currentView->scy)*currentView->w],m,font);
	}
	
	curs_pos();
	// was:
	//if ((ox==currentView->cx)&&(oy==currentView->cy))
	// but it lead to shadows during moving cursor beyound borders (JH)
	
	if ((ox==currentView->cx)||(oy==currentView->cy))
		draw_screen(1);
	else draw_cursor();
	return 1;
}

int askRawFormat(char * aquestion)
{
switch (ask(aquestion,"Bi*t|*Byte"))
	{
		case 1: return FILE_RAWBITMASK; 
		case 2: return FILE_RAWMASK;
	}
	return 0;	
}

void fill_draw_char(view * currentView, unsigned char f, int find)
{
	int i,j,r;
	unsigned char * look;
	j=currentView->w*currentView->h;
	look=currentView->map;
	for(i=0;i<j;i++) {
		if (!find || (*look==f)) {
			r=(int)(100.0*UNI);
			if (r<ratio) *look=currentView->dc[0];
		}
		look++;
	}
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
	int i,f;
	char *fname;


	if (typeMode) {
		if (!sym) {
			currentView->map[(currentView->cx+currentView->scx)+(currentView->cy+currentView->scy)*currentView->w]=cmd;
			draw_cursor();
			SDLCharBlt(MainContext,currentView->cx*currentView->cw,MAP_TOP_OFFSET+currentView->cy*currentView->ch,cmd);
			draw_cursor();
			move(1,0);
			cmd=0;
		}
	}

	if (sym) {
		switch(sym) {
			case SDLK_ESCAPE: {
						  if (typeMode) {
							  if (typeMode>1) hidden=0;
							  typeMode=0;
							  map_panel();
							  draw_header(1);
							  draw_screen(1);
							  return 0;
						  } else
							  bye();
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
	
	if (!is_command_allowed(cmd)) { return 0; }

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
		case '9':
			update_map_font(cmd-'0');
			draw_header(0);
			do_mode(mode);
			break;
		case 'h': 
			hidden=!hidden;
			cacheOk=0;
			break;
			
		case 'f': 
			if TILE_MODE
				f=currentView->dc[0]=get_number("Tile to replace:", 1,255);
			else
				f=select_draw(24,32,"Select character to replace:",NULL,32,1);
			fill_draw_char(currentView, f, 1);

			cacheOk=0;
			draw_header(0);
			break;
			
		case 'c':
			if MASK_EDIT_MODE {
				memset(mask->map,0,mask->h*mask->w);
				break;
			} else {
				if (ratio==100) {
					memset(currentView->map,currentView->dc[0],currentView->w*currentView->h);
				} else {
					fill_draw_char(currentView, 0, 0);
				}
				cacheOk=0;
			}
			break;
			
		case 'r': 
			ratio=get_number("Enter ratio:",ratio,100);
			return 0;
			
		case 'd': 
			if TILE_MODE
				currentView->dc[0]=get_number("New draw tile:", currentView->dc[0],255);
			else
				select_draw(24,32,"Select draw char",currentView->dc,32,1);
			draw_header(0);
			cacheOk=0;
			break;
			
		case 'p':
			SDLBox(CONFIG.screenWidth-172,15,CONFIG.screenWidth-1,22,144);
			SDLstring(CONFIG.screenWidth-172,15,TILE_MODE?"Please probe the tile":"Please probe the char");
			do_probe=1;
			break;
		case 'm': 
			if MASK_EDIT_MODE {
				int i;
				unsigned char * look= currentView->map;
				unsigned char *store= mask->map;
				
				// this does not work well in automatic tiles
				// but when tile 0 is empty, this is good
				for (i=0; i<currentView->w*currentView->h; i++)
					*store++=*look++?1:0;
				
			} else {
				do {
					i=get_number("Enter ANTIC mode:",mode,-1);
				} while ((i>=0)&&((i<2)||(i>7)));
				if (mode==i) return 0;
				if (i>0) {
					currentView->cx=currentView->cy=currentView->scx=currentView->scy=0;
					mode=i;
					map_panel();
					draw_header(0);
					do_mode(mode);
				}
			}
			break;
			
		case 'z': 
			do_size(tileEditMode);
			draw_header(0);
			cacheOk=0;
			break;
			
		case 'i': {
			int itsx, itsy;
			if (tileEditMode) {
				break;
			}
			
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
			set_mapview(VIEW_TILE);
			if ((tsy>1)||(tsx>1)) {
				untile_map(map,tile);
			} else {
				tile_map(itsx,itsy,map,tile);
			}
			set_mapview(VIEW_MAP);
			map_panel();
			draw_header(0);
			break;
		}
		case 'U':
			
			if (base) base=0;
			else base=64*8;
			map_panel();
			do_mode(mode);
			
			break;
			
		case 'e': 
			return 1;
			
		case 's':
			if MASK_EDIT_MODE {
				memset(mask->map,1,mask->h*mask->w);
				break;
			} else 
				OBJECT_IO("Save map:",1,save_map_file_name,"Map saved.",write_map(fname,font,map,FILE_NATIVE))
			
			return 0;
		case 'w':
			if MASK_EDIT_MODE {
				int rawformat=askRawFormat("Raw mask output format?");
				if (rawformat)
					OBJECT_IO("Write raw mask:",1,save_raw_mask_file_name,"Raw mask written.",write_map(fname,font,map,rawformat))
				
			} else {
				OBJECT_IO("Write raw map:",1,save_raw_map_file_name,"Raw map written.",write_map(fname,font,map,FILE_RAWMAP))
			}
			return 0;
		case 'l':
			OBJECT_IO("Load map:",0,save_map_file_name,0,read_map(fname,font,map,FILE_NATIVE))
			draw_header(0);
			do_mode(mode);
			break;
			
		case 'R':
			if MASK_EDIT_MODE {
				int rawformat=askRawFormat("Raw mask input format?");
				if (rawformat)
					OBJECT_IO("Read raw mask:",0,save_raw_mask_file_name,0,read_map(fname,font,map,rawformat))
				draw_header(0);
				do_mode(mode);

			} else {
				OBJECT_IO("Read raw map:",0,save_raw_map_file_name,0,read_map(fname,font,map,FILE_RAWMAP))
				draw_header(0);
				do_mode(mode);
			}
			break;
			
		case 't': 
			if TILE_MODE
			{
				info_dialog("Type mode only in char mode!");
				break;
			}
			typeMode=1;
			if (!hidden) typeMode++;
			SDLBox(CONFIG.screenWidth-172,15,CONFIG.screenWidth-1,22,144);
			SDLstring(CONFIG.screenWidth-92,15,"Type Mode");
			cacheOk=0;
			hidden=1;
			break;
			
		case 'g': 
			do_move(currentView,tileEditMode);
			cacheOk=0;
			curs_pos();
			break;
			
		case 'b': 
			set_mapview(tileEditMode?VIEW_MAP:VIEW_TILE);
			map_panel();
			if (!cacheOk)
				draw_header(0);
			break;
		case 'a':
			cacheOk=0;
			draw_cursor();
			maskEditMode=!maskEditMode;
			cacheOk=0;
			draw_header(0);
			map_panel();
			
			//draw_screen(1);
			break;
			//draw_screen(1);
			//return 0;
			  
		default: 
			return 0;
			 
	}
	draw_screen(1);
	return 0;
}

int get_draw_color(view * currentView, int buttonnumber)
{
	if IN_RANGE(buttonnumber,1,3)
		return currentView->dc[buttonnumber-1];
	return currentView->dc[0];
}

void set_draw_color(view * currentView, int buttonnumber, int color)
{
	if IN_RANGE(buttonnumber,1,3)
		currentView->dc[buttonnumber-1]=color;
}


/*===========================================================================
 * map_click
 * process a mouse click
 * param x: x position of the mouse
 * param y: y position of the mouse
 * param buttonnumber: 1 if left mousebutton is pressed 2=middle, 3=right, 4 and more untested
 * param bright: 1 if left mousebutton is pressed
 * param down: returns mouse button status
 * returns: 0 if user is exiting map screen
 *==========================================================================*/
int map_click(int x, int y, int buttonnumber, int *down)
{
	int i,ox,oy;

	// menu at the right
	if ((!hidden)&&(IN_BOX(x,y,CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-5,CONFIG.screenWidth-5,MAP_TOP_OFFSET,cmds[0]))) {
		i=(y-MAP_TOP_OFFSET+10)/10;
		SDLrelease(); /* wait for mouse to unclick */
		*down=0;
		if (cmds[i]) return(map_command(cmds[i],0));
		return 0;
	}

	
	if (y>=MAP_TOP_OFFSET) {
		oy=i=(y-MAP_TOP_OFFSET)/currentView->ch;
		ox=x=x/currentView->cw;
		if ((ox+currentView->scx>=currentView->w)||
				(i+currentView->scy>=currentView->h))
			return 0;
		// with this "if" works much better, but still not in edit tile mode
		//if (ox!=currentView->cx || oy!=currentView->cy) {
			if (buttonnumber) {
				if (!typeMode) {
					if NOT_MASK_EDIT_MODE {
						if (do_probe)
						{
							set_draw_color(currentView,
										   buttonnumber,
										   currentView->map[(x+currentView->scx)+(i+currentView->scy)*currentView->w]
										   );
							do_probe=0;
							draw_header(0);
						}
						int drawcolor=get_draw_color(currentView, buttonnumber);
						// with this "if" works very well.
						if (currentView->map[(x+currentView->scx)+(i+currentView->scy)*currentView->w]!=drawcolor)
						{
							currentView->map[(x+currentView->scx)+(i+currentView->scy)*currentView->w]=drawcolor;
							x=x*currentView->cw;
							y=MAP_TOP_OFFSET+i*currentView->ch;
							
							SDLCharBlt(MainContext,x,y,drawcolor+ (TILE_MODE *256));
							
							if ((!hidden)&&(x+currentView->cw>CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-6))
								draw_screen(0);
						}
					}
					if MASK_EDIT_MODE {
						int setmaskfield=1;
						if (buttonnumber==MBUTTON_RIGHT) setmaskfield=0;
						if (buttonnumber==MBUTTON_LEFT) setmaskfield=1;
						
						if (mask->map[(x+mask->scx)+(i+mask->scy)*mask->w]!=setmaskfield)
						{
							mask->map[(x+mask->scx)+(i+mask->scy)*mask->w]=setmaskfield;
							//if (setmaskfield) draw_cursor();
							x=x*currentView->cw;
							y=MAP_TOP_OFFSET+i*currentView->ch;

							//if ((!hidden)&&(x+currentView->cw>CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-6))
							draw_screen(1);
						}
					}
					
				}
			}
			move(ox-currentView->cx,oy-currentView->cy);
		//}
	}
	return 0;
}

/*===========================================================================
 * draw_screen
 * redraw the screen view
 * param b: 1 if left mousebutton is pressed
 * returns: nothing useful
 *==========================================================================*/
int draw_screen(int b) /* b not used in this func */
{
	int i,w,h,m=0;
	int x,y,ofs,lx,ly,by,bx;
	unsigned char *max, *look, *check, *masklook=NULL;

	SDLNoUpdate();
	look=currentView->map+currentView->scy*currentView->w+currentView->scx;
	if MASK_EDIT_MODE {
		mask->scx=currentView->scx;
		mask->scy=currentView->scy;
		masklook=mask->map+mask->scy*mask->w+mask->scx;
	}

	max=currentView->map+currentView->w*currentView->h;
	lx=currentView->cx*currentView->cw;
	ly=currentView->cy*currentView->ch;

	w=currentView->cw*currentView->w;
	h=currentView->ch*currentView->h;
	if (w>CONFIG.screenWidth)
		w=CONFIG.screenWidth;
	if (h>CONFIG.screenHeight-MAP_TOP_OFFSET)
		h=CONFIG.screenHeight-MAP_TOP_OFFSET;

	ofs=currentView->w-((w+currentView->cw-1)/currentView->cw);
	by=y=0;
	check=cache;
	SDLClip(!hidden);

	
	while ((y<h)&&(!by)) {
		x=bx=0;
		while (x<w) {
			bx++;
			if (currentView->scx+bx>currentView->w) {
				SDLBox(x,MAP_TOP_OFFSET+y,CONFIG.screenWidth,MAP_TOP_OFFSET+y+currentView->ch,144);
				look++;
				break;
			}
			if (look<max) {
				i=*look++;
				if MASK_EDIT_MODE m=*masklook++;
			} else {
				by=y; break;
			} 
			if TILE_MODE
				i+=256;
			// this has to be solved in the future
			// if ((!cacheOk)||(*check!=i)) {
				SDLCharBlt(MainContext,x,MAP_TOP_OFFSET+y,i);
			
			if MASK_EDIT_MODE {
				if (m)
					SDLXORDottedBox(x,MAP_TOP_OFFSET+y, x+mask->cw-1,MAP_TOP_OFFSET+y+mask->ch-1);
			}
			//}
			
			x+=currentView->cw;
			*check++=i;
		}
		look+=ofs;
		if MASK_EDIT_MODE masklook+=ofs;
		y+=currentView->ch;
	}
	cacheOk=1;
	
	if (by) {
		SDLBox(0,by+MAP_TOP_OFFSET,CONFIG.screenWidth,CONFIG.screenHeight,144);
		SDLBox(0,by+MAP_TOP_OFFSET,CONFIG.screenWidth,by+MAP_TOP_OFFSET,0);
		cacheOk=0; /* should be able to avoid this by drawing last line */
	}
	if ((w<CONFIG.screenWidth)||(h<CONFIG.screenHeight-MAP_TOP_OFFSET)) {
		SDLBox(w,MAP_TOP_OFFSET,CONFIG.screenWidth,CONFIG.screenHeight,144);
		SDLBox(0,h+MAP_TOP_OFFSET,CONFIG.screenWidth,CONFIG.screenHeight,144);
	} 
	SDLClip(0);

	if (!hidden) {
		SDLBox(CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-8,MAP_TOP_OFFSET,CONFIG.screenWidth-1,CONFIG.screenHeight,0);
		SDLContextBlt(MainContext,CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-6,MAP_TOP_OFFSET,UpdContext,0,0,MAP_BUTTON_WIDTH*8+8,CONFIG.screenHeight);
	}
	draw_cursor();
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
	
	if (fontmode[bank]!=m) {
		fontmode[bank]=m;
		update_font(bank);
	}

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
	mask->cw=map->cw; mask->ch=map->ch;
	/* build characters... */
	setpal();
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
			tx=i%16;
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
	setdefaultpal();

	SDLSetContext(MainContext);

	return 1;
}

void plot_draw_char(int x, int y, unsigned char c) {
	char * dat;
	int clr;
	int m;

	m=get_8x8_mode(mode);
	SDLBox(x,y,x+7,y+7,clut[0]);
	
	if (mode>=6) {
		dat=(char *)(font+(c&63)*8+base);
		clr=(c>>6)+1;
		SDLCharEngine(x,y,c&63,1,clut[clr],(unsigned char *)dat);
	} else
		SDLmap_plotchr(x,y,c,m,font);
}

/*===========================================================================
 * draw_header
 * update map mode header
 * param update: if true, refresh the screen
 * returns: nothing useful
 *==========================================================================*/
int draw_header(int update) /* update not used */
{
	char buf[80];
	int m;
	m=get_8x8_mode(mode);
	SDLNoUpdate();
	SDLBox(0,6,CONFIG.screenWidth-1,22,144);
	SDLHLine(0,CONFIG.screenWidth-1,14,0);
	if (!tileEditMode) {
		SDLstring(8,6,"Map Editor - ");
		if MASK_EDIT_MODE {
			SDLBox(11*8,6,CONFIG.screenWidth-1,13,144);
			SDLstring(11*8,6,": Mask Edit Mode");
		}
		if ((currentView->w>999)||(currentView->h>999))
			sprintf(buf,"Mode %02d [%05dx%05d]",mode,currentView->w,currentView->h);
		else
			sprintf(buf,"Mode %02d [%03dx%03d]",mode,currentView->w,currentView->h);
	} else {
		SDLstring(8,6,"Tile Editor - ");
		sprintf(buf,"Size [%02dx%02d]",tsx,tsy);
	}
	SDLstring(9*8,15,buf);

	sprintf(buf,"Font %01d",bank);
	SDLstring(8,15,buf);
	
	
	if TILE_MODE {
		sprintf(buf,"Tile: %03d",currentView->dc[0]);
		SDLstring(CONFIG.screenWidth-92,15,buf);
	} else if NOT_MASK_EDIT_MODE {
		SDLstring(CONFIG.screenWidth-4-21*8,15,"Draw Char L:  M:  R: ");

		setpal();
		int i;
		for (i=0; i<3; i++)
			plot_draw_char(CONFIG.screenWidth-12-(2-i)*4*8, 15, currentView->dc[i]);
		setdefaultpal();
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
	set_mapview(VIEW_MAP);

	SDLClear(0);
	do_mode(mode);
	map_panel();
	draw_header(0);
	draw_screen(1);
	cacheOk=0;
	typeMode=0;

	do {
		SDLRedraw();

		SDL_WaitEvent(&event);

		switch(event.type){  /* Process the appropiate event type */
			case SDL_QUIT: {
					       done=map_command(0,SDLK_ESCAPE);
				       }
			case SDL_KEYDOWN: {
						  int sym=event.key.keysym.sym;
						  int ch=event.key.keysym.unicode&0x7f;

						  if ((sym==SDLK_ESCAPE)||(sym==SDLK_LEFT)||(sym==SDLK_RIGHT)||
								  (sym==SDLK_UP)||(sym==SDLK_DOWN)) {
							  done=map_command(ch,sym);
						  } else if (ch) {
							  if (typeMode) {
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
				
				
				int res;
				
				if ((!hidden)&&(IN_BOX(mx,my,CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-3,CONFIG.screenWidth-4,CONFIG.screenHeight-131,CONFIG.screenHeight-4))) {
								
					res=select_draw_event_loop(CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-3,CONFIG.screenHeight-131,currentView->dc,16,NULL);
					
					draw_header(0);
					//down=0;
				}
				
				if (!done) done=map_click(mx,my,event.button.button,&down);

				break;
			}
			case SDL_MOUSEMOTION: {
						      if (down) {
							      int mx,my,ox,oy;
							      mx=SDLTranslateClick(event.button.x);
							      my=SDLTranslateClick(event.button.y);
							      oy=(my-MAP_TOP_OFFSET)/currentView->ch;
							      ox=mx/currentView->cw;
							      if (((hidden)||(mx<CONFIG.screenWidth-MAP_BUTTON_WIDTH*8-6))&&((ox!=currentView->cx)||(oy!=currentView->cy))) {
								      done=map_click(mx,my,down,&down);
							      }
						      }
						      break;
					      }
			case SDL_MOUSEBUTTONUP:
				down=0;
				break;
			default:
						break;
		}
	} while(!done);
	set_mapview(VIEW_MAP);
	return 0;
}
/*=========================================================================*/
