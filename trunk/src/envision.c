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
#include <sys/types.h>

#include "SDL.h"

#include "SDLemu.h"
#include "colours.h"
#include "font.h"

#include "envision.h"
#include "preferences.h"

unsigned char *dfont, *font, *copy_from, *fontbank[10];
//char bank_mod[10];
int echr, bank, copy_size, values, act_col=1, back_col=0, menuPanel=1;

int cmds[32];
unsigned char undo[8];
unsigned char peek[64], plot[64];
opt options;

rgb_color colortable[256];

unsigned char clut[9]={0,40,202,148,70,0,0,0,0};
unsigned char clut_default[9]={0,40,202,148,70,0,0,0,0};
int orit[8]={128,64,32,16,8,4,2,1};
int andit[8]={127,191,223,239,247,251,253,254};

char * preferences_filepath;
char commands_allowed[64];


config CONFIG;

runtime_storage RUNTIME_STORAGE;

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

int  corneroffset[]={3,2,1,1,0,0,0,0};

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
		
		//SDLsetPalette(x,(colortable[x]>>16)&0xff,(colortable[x]>>8)&0xff,(colortable[x])&0xff);
		SDLsetPalette(x,colortable[x].r,colortable[x].g,colortable[x].b);
	}
}

void setdefaultpal()
{
	int x;

	for(x=0;x<256;x++) {
		
		//SDLsetPalette(x,(colortable[x]>>16)&0xff,(colortable[x]>>8)&0xff,(colortable[x])&0xff);
		SDLsetPalette(x,colortable_default[x*3],colortable_default[x*3+1],colortable_default[x*3+2]);
	}
}

void set_allowed_commands(int * cmds, int cmdcnt)
{
	int i;
	for (i=1; i<=cmdcnt; i++)
		commands_allowed[i-1]=cmds[i];
	commands_allowed[cmdcnt]='\0';
	
}

int is_command_allowed(int cmd)
{
	if (strchr(commands_allowed,cmd)) return 1;
	return 0;
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

	*sx=(c%32)*8+EDIT_CHARMAP_X;
	*sy=EDIT_CHARMAP_Y+(c>>5)*8;
	return 1;
}
/*===========================================================================
 * frame
 * draws frame around box
 * param x,y: coords of the frame
 * param width, height: dimensions of the frame
 * param col: inner color of the frame;
 * returns: nothing
 *==========================================================================*/

void frame(int x, int y, int width, int height, int col)
{
	SDLBox(x-2,y-2,x+width+1,y+height+1,4);
	SDLBox(x-1,y-1,x+width,y+height,2);
	SDLBox(x,y,x+width-1,y+height-1,col);
}

/*===========================================================================
 * show_char_typefaces
 * draws char set in various graphic modes with frame around
 * supposed to be painted on subview.
 * param echr: edited chr in current font;
 * returns: nothing
 *==========================================================================*/

void show_char_typefaces(int echr)
{
	
	frame(10,2,16,8,clut[0]);
	frame(10,15,16,16,clut[0]);
	frame(14,36,8,8,clut[0]);
	frame(14,49,8,16,clut[0]);
	setpal();
	SDLplotchr(10,2,echr,6,font);
	SDLplotchr(10,15,echr,7,font);
	SDLplotchr(14,36,echr,4,font);
	SDLplotchr(14,49,echr,5,font);
	setdefaultpal();
}

/*===========================================================================
 * update_color_chooser
 * updates the color chooser in sertain mode graphics
 * param act_col: actual draw color;
 *==========================================================================*/

void update_color_chooser(int act_col, int back_col){
	int j;
	setpal();
	for (j=0;j<4;++j)
		SDLBox(EDIT_GRID_X+(j<<4),EDIT_GRID_Y+68,EDIT_GRID_X+15+(j<<4),EDIT_GRID_Y+78,clut[j]);
	setdefaultpal();
	
	SDLHollowBox(EDIT_GRID_X+(back_col<<4),EDIT_GRID_Y+68,EDIT_GRID_X+15+(back_col<<4),EDIT_GRID_Y+78,2);
	SDLHollowBox(EDIT_GRID_X+(act_col<<4),EDIT_GRID_Y+68,EDIT_GRID_X+15+(act_col<<4),EDIT_GRID_Y+78,10);
}

/*===========================================================================
 * hex2dec
 * converts hex char to value
 * param hex: hex char to get value of; uppercase or lowercase;
 * returns value of hex char or <0 if error
 *==========================================================================*/

int hex2dec(char hex)
{
	hex=toupper(hex);
	if (hex=='\n') return -1;
	if (!((hex>='0' && hex<='9') ||(hex>='A' && hex<='F'))) return -2;
	if (hex<='9') return hex-'0';
	return hex-'A'+10;
}

int num2val(char * chrval)
{
	if (*chrval=='$')
		return strtol(++chrval,NULL,16);
	else
		return strtol(chrval,NULL,10);
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
	int x,y,clr,m;

	if (chr==echr) return 0;
	m=get_8x8_mode(mode);
	
	SDLNoUpdate();
	dat=font+(chr*8);

	if (rem) {
		memcpy(undo,dat,8);
	}
	frame(EDIT_GRID_X,EDIT_GRID_Y,64,64,0);

	if (m==2)
	{
		for(y=0;y<8;y++) {
			c=*(dat+y);
			for(x=0;x<8;x++) {
				if ((x+y)&1) clr=CONFIG.checkersLo; else clr=CONFIG.checkersHi;
				if (c&128) clr=CONFIG.whiteColor;
				SDLBox(EDIT_GRID_X+x*8,EDIT_GRID_Y+y*8,EDIT_GRID_X+x*8+7,EDIT_GRID_Y+7+y*8,clr);
				c=c<<1;
			}
		}
	}
	else if (m==4){
		setpal();
		for(y=0;y<8;y++) {
			c=*(dat+y);
			for(x=0;x<4;x++) {
				clr=clut[(c&0xc0)>>6];
				SDLBox(EDIT_GRID_X+x*16,EDIT_GRID_Y+y*8,EDIT_GRID_X+x*16+15,EDIT_GRID_Y+7+y*8,clr);
				c<<=2;
			}
		}
		setdefaultpal();
		update_color_chooser(act_col,back_col);
		
	}

	topos(echr,&x,&y);
	if (mode==4 || mode==5) setpal();
	SDLBox(x,y,x+7,y+7,clut[0]);
	SDLplotchr(x,y,echr,m,font);
	echr=chr;
	topos(echr,&x,&y);
	SDLBox(x,y,x+7,y+7,148);
	SDLplotchr(x,y,echr,m,font);
	setdefaultpal();

	SDLSetContext(UpdContext);
	SDLClear(0);
	
	show_char_typefaces(echr);
	
	SDLSetContext(MainContext);
	SDLContextBlt(MainContext,EDIT_GRID_X+72,EDIT_GRID_Y-2,UpdContext,0,0,32,66);
	
	
//	SDLBox(264,32,280,64,0); /*#*/
	SDLBox(EDIT_GRID_X+18,EDIT_GRID_Y+82,EDIT_GRID_X+91,EDIT_GRID_Y+89,0);
	sprintf(num,":$%02x/%d",echr,echr);
	SDLstring(EDIT_GRID_X+18,EDIT_GRID_Y+82,num);
	frame(EDIT_GRID_X+86,EDIT_GRID_Y+82,8,8,0);
	SDLplotchr(EDIT_GRID_X+86,EDIT_GRID_Y+82,echr,2,dfont);
	
	draw_numbers(values,dat);
	SDLUpdate();

	corner(0);
	return 1;
}

void clr_ext_cmd()
{
	SDLBox(EDIT_CHARMAP_X,EDIT_CHARMAP_Y-10,EDIT_CHARMAP_X+152,EDIT_CHARMAP_Y-3,0);
}
/*===========================================================================
 * panel
 * update command panel
 * param tp: the panel to show
 * returns: flag indicating character change
 *==========================================================================*/
int panel(int tp)
{
	int idxcnt;
	menuPanel=tp;
	SDLSetContext(UpdContext);
	SDLClear(0);
	idxcnt=0;
	drawbutton(0,idxcnt*10+8,"Go to *Map"); cmds[++idxcnt]='m';
	drawbutton(0,idxcnt*10+8,"*Blank"); cmds[++idxcnt]='b';
	drawbutton(0,idxcnt*10+8,"*inverse"); cmds[++idxcnt]='i';
	drawbutton(0,idxcnt*10+8,"*Undo"); cmds[++idxcnt]='u';
	drawbutton(0,idxcnt*10+8,"*Atari"); cmds[++idxcnt]='a';
	drawbutton(0,idxcnt*10+8,"Flip *Horiz"); cmds[++idxcnt]='h';
	drawbutton(0,idxcnt*10+8,"Flip *Vert"); cmds[++idxcnt]='v';
	drawbutton(0,idxcnt*10+8,"*Rotate"); cmds[++idxcnt]='r';
	drawbutton(0,idxcnt*10+8,"*Copy"); cmds[++idxcnt]='c';
	drawbutton(0,idxcnt*10+8,"*X-copy"); cmds[++idxcnt]='x';
	drawbutton(0,idxcnt*10+8,"*Transcopy"); cmds[++idxcnt]='t';
	drawbutton(0,idxcnt*10+8,"*Pick Colors"); cmds[++idxcnt]='p';
	//drawbutton(0,idxcnt*10+8,"RestorFont"); cmds[++idxcnt]='A';
	drawbutton(0,idxcnt*10+8,"*Save Font"); cmds[++idxcnt]='s';
	drawbutton(0,idxcnt*10+8,"*Load Font"); cmds[++idxcnt]='l';
	drawbutton(0,idxcnt*10+8,"*Export"); cmds[++idxcnt]='e';
	drawbutton(0,idxcnt*10+8,"*Import Pall"); cmds[++idxcnt]='I';
	drawbutton(0,idxcnt*10+8,"*Options"); cmds[++idxcnt]='o';
	drawbutton(0,idxcnt*10+8,"De*faults"); cmds[++idxcnt]='f';
	// dfgjknqwyz
	cmds[0]=idxcnt*10+8;
	SDLSetContext(MainContext);
	SDLContextBlt(MainContext,EDIT_MENU_X,EDIT_MENU_Y,UpdContext,0,0,BUTTON_WIDTH*8,cmds[0]);
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
	SDLNoUpdate();
	frame(EDIT_CORNER_X,EDIT_CORNER_Y,64,64,0);
	if (mode>=4) setpal();
	if (all) {
		SDLSetContext(BackContext);
		SDLBox(0,0,63,63,clut[0]);
	} else {
		SDLSetContext(UpdContext);
		SDLBox(0,0,w*4,h,clut[0]);
		// hack
		SDLmap_plotchr(0,0,match,mode,font);
		SDLmap_plotchr(w,0,match+64,mode,font);
		SDLmap_plotchr(w*2,0,match+128,mode,font);
		SDLmap_plotchr(w*3,0,match+192,mode,font);
	}

	memset(peek,0,64);
	memset(plot,0,64);
	i=0;
	// peek = got fragment of the map 8x8
	// plot = flag showing whenewer there is char or empty wield
	int mx=currentView->cx/*(currentView->ch/8)*/+currentView->scx; //-corneroffset[tsx];
	int my=currentView->cy/*(currentView->cw/8)*/+currentView->scy; //-corneroffset[tsy];
	if (mx<0) mx=0;
	if (my<0) my=0;
	//if (tsx==2) if (mx>(currentView->w/tsx)*tsx-8/tsx) mx=(currentView->w/tsx)*tsx - 8/tsx;
	//if (tsx>=4) if (mx>currentView->w-2) mx=currentView->w - 2;
	//if (my>currentView->h-8) my=currentView->h-8;
	//if (tsy>=4) if (my>currentView->h-2) my=currentView->h - 2;
	if (mx>currentView->w) mx=currentView->w;
	if (my>currentView->h) my=currentView->h;
	for(y=0;y<8;y++) {
		x=0;
		if (my+y>=currentView->h)
			break;
		look=&currentView->map[mx+x+(my+y)*currentView->w];
		i=y*8;
		for(x=0;x<8;x++) {
			if (mx+x>=currentView->w)
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
	// copying to the screen
	while (y<64) {
		look=&peek[idx<<3];
		check=&plot[idx<<3];
		x=0;
		while (x<64) {
			i=*look++;
			if (*check++) { // if tile is to be copied
				if (all) { // what is all?
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
	setdefaultpal();
	SDLSetContext(MainContext);
	//if (f)
		SDLContextBlt(MainContext,EDIT_CORNER_X,EDIT_CORNER_Y,BackContext,0,0,63,63);
	SDLUpdate();
	
	
	return 1;
}

int colors()
{
	char buf[32];
	int i;

	SDLNoUpdate();
	SDLBox(EDIT_COLOR_X-3,EDIT_COLOR_Y,EDIT_COLOR_X+4*44+36,EDIT_COLOR_Y+33,0);
	
	//SDLstring(tmpx+18,yp,"Color Registers");
	for(i=0;i<5;i++) {
		SDLHollowBox(EDIT_COLOR_X+i*44,EDIT_COLOR_Y+17,EDIT_COLOR_X+i*44+33,EDIT_COLOR_Y+33
					 ,144);
		setpal();
		SDLBox(EDIT_COLOR_X+i*44+1,EDIT_COLOR_Y+18,EDIT_COLOR_X+i*44+32,EDIT_COLOR_Y+32,clut[i]);
		setdefaultpal();
		
		sprintf(buf,"PF%d",(int)i);
		SDLstring(EDIT_COLOR_X+i*44+5
				  ,EDIT_COLOR_Y,buf);
		switch (CONFIG.color_display_mode) {
			case COLOR_DISPLAY_HEX:
				sprintf(buf,"$%02X",clut[i]);
				SDLstring(EDIT_COLOR_X+i*44+5,EDIT_COLOR_Y+8,buf);
				break;
			case COLOR_DISPLAY_DEC:
				sprintf(buf,"%03d",clut[i]);
				SDLstring(EDIT_COLOR_X+i*44+5,EDIT_COLOR_Y+8,buf);
				break;
			default:
				break;
		}
		//sprintf(buf,"%d",clut[i]);
	}
	
	SDLUpdate();
	setdefaultpal();
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
	int i,sx,sy,m;

	font=fontbank[b];
	bank=b;
	SDLSetContext(DialogContext);
	SDLNoUpdate();
	if (mode==4 || mode==5) setpal();
	SDLClear(0);
	sx=0; sy=0;
	m=get_8x8_mode(mode);
	for(i=0;i<128;i++) {
		SDLplotchr(sx,sy,i,m,font);
		sx+=8;
		if (sx>248) { sx=0; sy+=8; }
	}
	SDLSetContext(MainContext);
	frame(EDIT_CHARMAP_X,EDIT_CHARMAP_Y,256,32,0);
	// copying painted fonts to the main screen
	SDLContextBlt(MainContext,EDIT_CHARMAP_X,EDIT_CHARMAP_Y,DialogContext,0,0,255,31);
	corner(1);

	// font set number plotting
	SDLBox(EDIT_FONTSEL_X+12,EDIT_FONTSEL_Y+8,EDIT_FONTSEL_X+20,EDIT_FONTSEL_Y+15,144);
	SDLplotchr(EDIT_FONTSEL_X+12,EDIT_FONTSEL_Y+8,16+b,1,dfont);
	SDLUpdate();
	setdefaultpal();
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
	// unpack(titles,0,0);
	SDLstring(EDIT_GRID_X-14,EDIT_GRID_Y+82,"Char");
	
	SDLBox(EDIT_FONTSEL_X-1,EDIT_FONTSEL_Y,EDIT_FONTSEL_X+33,EDIT_FONTSEL_Y+15,144);
	SDLstring(EDIT_FONTSEL_X,EDIT_FONTSEL_Y,"Font");
	SDLplotchr(EDIT_FONTSEL_X,EDIT_FONTSEL_Y+8,126,1,dfont);
	SDLplotchr(EDIT_FONTSEL_X+24,EDIT_FONTSEL_Y+8,127,1,dfont);

	SDLBox(EDIT_MODESEL_X-1,EDIT_MODESEL_Y,EDIT_MODESEL_X+33,EDIT_MODESEL_Y+15,144);
	SDLstring(EDIT_MODESEL_X,EDIT_MODESEL_Y,"Mode");
	SDLplotchr(EDIT_MODESEL_X,EDIT_MODESEL_Y+8,126,1,dfont);
	SDLplotchr(EDIT_MODESEL_X+24,EDIT_MODESEL_Y+8,127,1,dfont);
	SDLplotchr(EDIT_MODESEL_X+12,EDIT_MODESEL_Y+8,16+mode,1,dfont);
	
	update_font(bank);
	i=echr;
	echr=0;
	grid(i,1);
	panel(1);
	colors();
	SDLUpdate();
	return 1;
}

view * map_init(int alloc_map, view * map, int width, int height) {
	if (!map) {
		map=(view *)malloc(sizeof(view));
		map->map=NULL;
	}
	map->w=width; map->h=height;
	map->ch=map->cw=8;
	map->dc=1;
	map->cx=map->cy=map->scx=map->scy=0;
	if (map->map) free(map->map);
	map->map=NULL;
	if (alloc_map) {
		map->map=(unsigned char *)malloc(map->w*map->h);
		memset(map->map,0,map->w*map->h);
	}
	return map;
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
	int i, ce;
	

	ce=0;
	strcpy(CONFIG_ENTRIES[ce].pref_id,"MAIN_SETUP");
	CONFIG_ENTRIES[ce].len=sizeof(CONFIG);
	CONFIG_ENTRIES[ce].init_handler=handler_config_reset;
	CONFIG_ENTRIES[ce++].buffer=&CONFIG;
	
	strcpy(CONFIG_ENTRIES[ce].pref_id,"COLOR_TABLE");
	CONFIG_ENTRIES[ce].len=sizeof(colortable);
	CONFIG_ENTRIES[ce].init_handler=handler_palette_reset;
	CONFIG_ENTRIES[ce++].buffer=colortable;

	strcpy(CONFIG_ENTRIES[ce].pref_id,"CLUT");
	CONFIG_ENTRIES[ce].len=sizeof(clut);
	CONFIG_ENTRIES[ce].init_handler=handler_clut_reset;
	CONFIG_ENTRIES[ce++].buffer=clut;

	for (i=0; i<sizeof(CONFIG_ENTRIES) / sizeof (CONFIG_ENTRY); i++)
		CONFIG_ENTRIES[i].init_handler();
	
	
	// getting preferences saved, this will overwrite default values set above
	getprefs();
	// setting new full set of preferences.
	setprefs();

	memset(&RUNTIME_STORAGE, 0, sizeof (RUNTIME_STORAGE));
	
	if (!initSDL(zoom,fullScreen))
		return 0;

	
	dfont=FONT;
	font=fontbank[0];

	for(i=0;i<10;i++) {
		fontbank[i]=(unsigned char *)malloc(1024);
		//bank_mod[i]=0;
		memcpy(fontbank[i],dfont,1024);
	}


	copy_from=cache=NULL;
	options.disk_image=NULL;
	options.base=options.step=0;
	options.write_tp=1;
	base=bank=0;
	ratio=100;
	mode=2;
	echr=33;
	menuPanel=1;
	memset(peek,0,64);
	memset(plot,0,64);

	map=map_init(MAP_ALLOC,NULL,CONFIG.defaultMapWidth,CONFIG.defaultMapHeight);
	
	currentView=map;

	mask=map_init(MAP_ALLOC,NULL,CONFIG.defaultMapWidth,CONFIG.defaultMapHeight);
	
	for(i=0;i<256;i++)
		map->map[i]=i;

	tsx=1; tsy=1;
	
	tile = map_init(MAP_ALLOC,NULL,16, 16);
	
	for(i=0;i<256;i++)
		tile->map[i]=i;

	for(i=0;i<512;i++)
		charTable[i]=NULL;

	// visible area ?
	cache=(unsigned char *)malloc((CONFIG.screenWidth/8)*(CONFIG.screenHeight/8));
	cacheOk=0;

	s1=time(NULL);
	s2=0x01234567^s1;

	title();

	setdefaultpal();
	draw_edit();
	if (get_error())	error_dialog(get_error());

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

int update(int x, int y)
{
	topos(echr,&x,&y);
	SDLSetContext(UpdContext);
	setpal();
	SDLBox(0,0,7,7,clut[0]);
	SDLplotchr(0,0,echr,mode,font);
	SDLSetContext(MainContext);
	SDLContextBlt(MainContext,x,y,UpdContext,0,0,7,7);
	setdefaultpal();
	
	SDLSetContext(UpdContext);
	SDLClear(0);
	
	show_char_typefaces(echr);
	
	SDLSetContext(MainContext);
	SDLContextBlt(MainContext,EDIT_GRID_X+72,EDIT_GRID_Y-2,UpdContext,0,0,32,66);
	corner(0);
	return 1;
	
}

/*===========================================================================
 * update2bits
 * update a character int 2-bit mode, reflecting change on map, etc.
 * param x: grid x-position to change
 * param y: grid y-position to change
 * param c: color (0-3)
 * returns: nothing useful
 *==========================================================================*/
int update2bits(int x, int y, int c)
{
	unsigned char *dat;
	int shift;
	
	dat=font+echr*8+y;
	shift=(6-((x&3)<<1));
	*dat=*dat&~(3<<shift);
	*dat=*dat|(c<<shift);
	
	return update(x,y);
}

int update1bit(int x, int y, int c)
{
	unsigned char *dat;

	dat=font+echr*8+y;
	if (c) *dat=*dat|orit[x];
	else *dat=*dat&andit[x];
	return update(x,y);
}


/*===========================================================================
 * click
 * handle a click on the edit screen
 * param x: x-position of click
 * param y: y-position of click
 * param b: b is if left mousebutton is clicked
 * returns: nothing useful
 *==========================================================================*/
int click(int x, int y, int b)
{
	SDL_Event event;
	int i,ox,oy,done;

	if (b!=1)
		b=0;

	// select panel 2 Disk
	/*
	if ((menuPanel==1)&&IN_BOX(x-EDIT_MENU_X,y-EDIT_MENU_Y,33,63,3,7)) {
		panel(2);
		return 0;
	// select panel 1 Edit
	} else if ((menuPanel==2)&&IN_BOX(x-EDIT_MENU_X,y-EDIT_MENU_Y,1,30,3,7)) {
		panel(1);
		return 0;
	} else*/
	
	for(i=0;i<5;i++) {
		if (IN_BOX(x-EDIT_COLOR_X,y-EDIT_COLOR_Y,i*44,i*44+33,17,33))
		{
			if (show_palette(i,EDIT_COLOR_X+i*44, EDIT_COLOR_Y+33))
			{
				colors();
				draw_edit();
			}
		//sprintf(buf,"%d",clut[i]);
		return 0;
		}
		if (IN_BOX(x-EDIT_COLOR_X,y-EDIT_COLOR_Y,i*44,i*44+33,0,16))
		{
			CONFIG.color_display_mode++;
			if (CONFIG.color_display_mode>=COLOR_DISPLAY_MAX) CONFIG.color_display_mode=0;			
			colors();
			return 0;
		}
		
	}
	
	
	// decrease font set number
	if (IN_BOX(x,y,EDIT_FONTSEL_X,EDIT_FONTSEL_X+8,EDIT_FONTSEL_Y+8,EDIT_FONTSEL_Y+16)) {
		bank--;
		if (bank<0) bank=9;
		command(48+bank,0);
		return 0;
	// increase font set number
	} else if (IN_BOX(x-EDIT_FONTSEL_X,y-EDIT_FONTSEL_Y,24,32,8,16)) {
		bank++;
		if (bank>9) bank=0;
		command(48+bank,0);
		return 0;
		// decrease mode
	} else if (IN_BOX(x-EDIT_MODESEL_X,y-EDIT_MODESEL_Y,0,8,8,16)) {
		if (mode>2) mode--;
		do_mode(mode);
		draw_edit();
		return 0;
		// increase mode
	} else if (IN_BOX(x-EDIT_MODESEL_X,y-EDIT_MODESEL_Y,24,32,8,16)) {
		if (mode<7) mode++;
		do_mode(mode);
		draw_edit();
		return 0;
		// menu 
	} else if (IN_BOX(x-EDIT_MENU_X,y-EDIT_MENU_Y,0,64,0,cmds[0])) {
		i=(y-EDIT_MENU_Y+2)/10;
		y=(y/10)*10;
		SDLrelease();
		if (copy_from) {
			copy_from=NULL;
			copy_size=0;
			SDLNoUpdate();
			clr_ext_cmd();
			SDLUpdate();
		}
		if (cmds[i]) command(cmds[i],0);
		return 0;
	}

	// Click on the corner map in edit char mode
	if (IN_BOX(x-EDIT_CORNER_X,y-EDIT_CORNER_Y,0,63,0,63)) {
		x=(x-EDIT_CORNER_X)/8;
		if (mode==3) y=(y-EDIT_CORNER_Y)/10;
		else y=(y-EDIT_CORNER_Y)/8;
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
	do { // click in font area
		if (IN_BOX(x-EDIT_CHARMAP_X,y-EDIT_CHARMAP_Y,0,255,0,31)) {
			x=(x-EDIT_CHARMAP_X)/8; y=(y-EDIT_CHARMAP_Y)/8;
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
					clr_ext_cmd();
					if (copy_size>8) update_font(bank);
					grid(i,0);
					copy_from=NULL;
					copy_size=0;
					return 0;
				} else {
					x=font+i*8-copy_from+8;
					if ((x<0)||(x>1024)) return 0;
					else if (x==0) {
						clr_ext_cmd();
						copy_from=NULL;
						copy_size=0;
						return 0;
					}
					copy_size=x;
					SDLNoUpdate();
					clr_ext_cmd();
					SDLstring(EDIT_CHARMAP_X,EDIT_CHARMAP_Y-10,"Copy range to:");
					SDLUpdate();
				}
				return 0;
			}
			grid(i,1);
		} else { //char edit
		/*
			 * mode 2 - 8x8 2 col 8x8
			 * mode 3 - 8x8 2 col 8x10	wrap to 8x8
			 * mode 4 - 4x8 4 col 8x8	
			 * mode 5 - 4x8 4 col 8x16	shrink height 
			 * mode 6 - 8x8 4 col 16x8  shrink width
			 * mode 7 - 8x8 4 col 16x16 shrink height and width
			 */
			if (IN_BOX(x-EDIT_GRID_X,y-EDIT_GRID_Y,0,63,0,63)) {
				const int xe=x&0xfffffff8;
				const int ye=y&0xfffffff8;
				int col44,bit4,xe4;
				int grid_checker;
				switch (mode)
				{
					case 2:
					case 3:
					case 6:
					case 7:
						// I hate such constructs (JH)
						//i=10+(1-b)*(138-((((xe>>3)+(ye>>3))&1)<<2));//true 144 false 148
						grid_checker=(xe&8)^(ye&8);
						if (b)
							i=CONFIG.whiteColor;
						else 
							i=(grid_checker?CONFIG.checkersLo:CONFIG.checkersHi);
						SDLNoUpdate();
						SDLBox(xe,ye,xe+7,ye+7,i);
						update1bit((xe-EDIT_GRID_X)>>3,(ye-EDIT_GRID_Y)>>3,b);
						SDLUpdate();
						break;
					case 4:
					case 5:
						xe4=xe&0xfffffff0;
						if (b)
						{
							col44=clut[act_col];
							bit4=act_col;
						}
						else
						{
							col44=clut[back_col];
							bit4=back_col;
						}
						SDLNoUpdate();
						SDLBox(xe4,ye,xe4+15,ye+7,col44);
						update2bits((xe4-EDIT_GRID_X)>>4,(ye-EDIT_GRID_Y)>>3,bit4&3);
						SDLUpdate();

				};


			}
			
			if (mode==4 || mode==5)
			{
				const int xe=x&0xfffffff0;
				SDLNoUpdate();
				if (IN_BOX(x-EDIT_GRID_X,y-EDIT_GRID_Y,0,63,68,78))
				{
					if (b)
						act_col=(xe-EDIT_GRID_X)>>4;
					else
						back_col=(xe-EDIT_GRID_X)>>4;
				}
				update_color_chooser(act_col,back_col);
				
				SDLUpdate();

			}
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

void draw_numbers(int vals, unsigned char *work){
	int i;
	char buf[4];
	SDLNoUpdate();
	SDLBox(EDIT_GRID_X-28,EDIT_GRID_Y,EDIT_GRID_X-3,EDIT_GRID_Y+64,0);
	if (vals) {
		for(i=0;i<8;i++) {
			if (vals==2)
				sprintf(buf,"$%02x",work[i]);
			else
				sprintf(buf,"%3d",work[i]);
			SDLstring(EDIT_GRID_X-26,EDIT_GRID_Y+i*8,buf);
		}
	}
	SDLUpdate();
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
	char *fname;

	dat=font+echr*8;
	memcpy(work,dat,8);

	if (sym) {
		switch(sym) {
			case SDLK_ESCAPE:
				bye();
				break;
			case SDLK_LEFT:
				left(dat,work);
				break;
			case SDLK_RIGHT:
				right(dat,work);
				break;
			case SDLK_UP:
				up(dat,work);
				break;
			case SDLK_DOWN:
				down(dat,work);
				break;
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
		case '9':
				  update_font(cmd-'0');
				  break;
		case 'b':
				  memset(dat,0,8);
				  break;
		case 'i':
				  for(i=0;i<8;i++)
					  *(dat+i)=255-*(dat+i);
                  break;
		case 'a':
				  memcpy(dat,dfont+echr*8,8);
					break;
		case 'v':
				  for(i=0;i<8;i++)
					  *(dat+i)=work[7-i];
				  break;
		case 'r':
				  rotate8x8(work,1,dat,1);
				  break;
		case 'u':
				  memcpy(dat,undo,8);
				  memcpy(undo,work,8);
                  break;
		case 'h':
				  for(i=0;i<8;i++) {
					  l=work[i];
					  *(dat+i)=0;
					  for(j=0;j<8;j++)
						  if (l&orit[j]) *(dat+i)+=orit[7-j];
				  }
				  break;
		case 'c':
				  copy_from=dat;
				  copy_size=8;
				  SDLNoUpdate();
				  clr_ext_cmd();
				  SDLstring(EDIT_CHARMAP_X,EDIT_CHARMAP_Y-10,"Copy to:");
				  SDLUpdate();
				  break;
		case 'x':
				  copy_from=dat;
				  copy_size=0;
				  SDLNoUpdate();
				  clr_ext_cmd();
				  SDLstring(EDIT_CHARMAP_X,EDIT_CHARMAP_Y-10,"Select range end:");
				  SDLUpdate();
				  break;
			  
		case 't':
				  copy_from=dat;
				  copy_size=-8;
				  SDLNoUpdate();
				  clr_ext_cmd();
				  SDLstring(EDIT_CHARMAP_X,EDIT_CHARMAP_Y-10,"Transcopy to:");
				  SDLUpdate();
				  break;
		case 'A':
			memcpy(font,dfont,1024);
			update_font(bank);
			break;
		case 'f':
			do_defaults();
				  break;
		case 'n':
				  values++;
				  if (values==3) values=0;
				  break;
		case 's':
				  fname=get_filename("Save font:",options.disk_image,RUNTIME_STORAGE.save_font_file_name);
				  if (fname) {
					  strncpy(RUNTIME_STORAGE.save_font_file_name,fname,127);
					  if (options.disk_image)
						  i=write_xfd_font(options.disk_image,fname,font,1024,NULL);
					  else
						  i=write_font(fname,font);
					  free(fname);
				  }
				  break;
		case 'l':				  fname=get_filename("Load font:",options.disk_image,NULL);
				  if (fname) {
					  strncpy(RUNTIME_STORAGE.save_font_file_name,fname,127);
					  if (options.disk_image)
						  i=read_xfd_font(options.disk_image,fname,font,1024);
					  else
						  i=read_font(fname,font);
					  free(fname);
					  update_font(bank);
				  }
				  break;
		case 'I':
			fname=get_filename("Import color table:",0,NULL);
			if (fname) {
				if (import_palette(fname,colortable)) {
					setpal();
					setprefs();
					draw_edit();
				}
				free(fname);
				
			
			}
			break;
			
		case 'e':
				  fname=get_filename("Export font:",options.disk_image,RUNTIME_STORAGE.export_font_file_name);
				  if (fname) {
					  strncpy(RUNTIME_STORAGE.export_font_file_name,fname,127);
					  if (options.disk_image)
						  i=write_xfd_data(options.disk_image,fname,font,0,127);
					  else
						  i=write_data(fname,font,0,127,0);
					  free(fname);
				  }
				  break;
		case 'o':
			do_options();
			
			break;
		case 'p':
				do_colors();
				update_font(bank);
				//corner(1);
			colors();
				break;
		case 'm':				  do_map();
				  draw_edit();
				  return 0;
/*		case 'f': {
			toggleFullScreen();
			draw_edit();
			
			break;
		}
*/			
		default:				 /*  printf("%d ",cmd); */
				 if (!sym)
					 return 0;
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

						  //if ((sym=='q')&&(event.key.keysym.mod&KMOD_CTRL)) {
							//  sym=SDLK_ESCAPE;  /* handle ctrl-q */
						  //}
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
	
	preferences_filepath=get_preferences_filepath();

	zoom=2;
	fullScreen=0;

	for(i=1;i<argc;i++) {
		if ((!strcmp(argv[i],"-z"))||(!strcmp(argv[i],"-zoom"))) {
			if (i<argc-1) {
				i++;
				zoom=num2val(argv[i]);
				if (zoom<1)
					zoom=2;
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
