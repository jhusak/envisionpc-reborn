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
#include "colours.h"
#include "envision.h"
#include "preferences.h"

int drawbevelledbox(int x, int y, int w, int h)
{
	return raisedbox(x, y, x+w, y+h,0);
}

/*===========================================================================
 * drawbutton
 * draw a push button
 * param x: upper left x-position of button
 * param y: upper left y-position of button
 * param txt: button text
 * returns: nothing useful
 *==========================================================================*/

int drawbuttonwidth(int x, int y, char *txt, int width,int height)
{
	int l,o,c;
	char *f,btxt[32];

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
	l=width*4-strlen(btxt)*4;
	drawbevelledbox(x,y,width*8+5,height-1);
	SDLstring(x+l+2,y,btxt);
	if (c) {
		if (c<96) c-=32;
		SDLplotchr(x+l+2+o,y,c,1,dfont);
	}
	return 1;
}

int drawbutton(int x, int y, char *txt)
{
	return drawbuttonwidth(x,y,txt,BUTTON_WIDTH,9);
}
int drawbutton_map(int x, int y, char *txt)
{
	return drawbuttonwidth(x,y,txt,MAP_BUTTON_WIDTH,9);
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

int openDblBufferDialog(int x, int y, int w, int h, char * title, int hidecursor)
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
	raisedbox(x,y,x+w,y+h,3);
	dblBufferTmpStorage.x=x;
	dblBufferTmpStorage.y=y;
	dblBufferTmpStorage.w=w+2;
	dblBufferTmpStorage.h=h+2;
	
	if (hidecursor) SDL_ShowCursor(SDL_DISABLE);
	SDLstring(x+(w/2-strlen(title)*4)&(~0x7),y+2,title);
	
	return x;
}

void closeLastDblBufferDialog()
{
	SDLContextBlt(MainContext,dblBufferTmpStorage.x,dblBufferTmpStorage.y,DialogContext,0,0,dblBufferTmpStorage.w,dblBufferTmpStorage.h);
	SDL_ShowCursor(SDL_ENABLE);
	SDLRedraw();
}

/*===========================================================================
 * raisedbox
 * draw a raised box
 * param x: upper left x-position of box
 * param y: upper left y-position of box
 * param w: box width
 * param h: box height
 * param d: box depth
 * returns: computed x of the box or passed through
 *==========================================================================*/
int raisedbox(int x, int y, int w, int h, int d)
{
	int i;
	w=w-x;
	h=h-y;
	
	
	SDLBox(x+1,y+1,x+w-1,y+h-1,148);
	SDLLine(x,y,x+w,y,152);
	SDLLine(x+w,y+1,x+w,y+h,144);
	SDLLine(x,y+h,x+w,y+h,144);
	SDLLine(x,y+1,x,y+h,152);
	for (i=1; i<d; i++) {
		SDLLine(x+w+i,y+i,x+w+i,y+h+i,0);
		SDLLine(x+i,y+h+i,x+w+i,y+h+i,0);
	}
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

	SDLNoUpdate();
	SDL_ShowCursor(SDL_DISABLE);
	SDLBox(xp,yp,slen,yp+8,146);
	SDLHLine(xp-1,slen+1,yp-1,144);
	SDLHLine(xp-1,slen+1,yp+9,152);
	SDLVLine(xp-1,yp,yp+8,144);
	SDLVLine(slen+1,yp,yp+8,152);

	if (init && *init) {
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
	int first=1;

	do {
		SDLVLine(cx,yp,yp+7,cc);
		c=SDLgetch(0);
		if (!c) c=SDLgetch(0);
		if (!c) c=SDLgetch(0);
		if (!c) c=SDLgetch(0);
		if (!c) c=SDLgetch(0);
		if (!c) c=SDLgetch(0);

		if (!c) {
			c=SDLgetch(0);
		} else {
			if (c==27) { SDL_ShowCursor(SDL_ENABLE);SDLRedraw(); return NULL;}
			if (first && c!=8 && c!=13)
			{
				first=0;
				cp=0;
				cx=xp+1+8*cp;
				SDLBox(cx,yp,slen,yp+7,146);
				buffer[cp]=0;
				cc=10;
				dd=500;
			}
			
			if ((c==8)&&(cp)) // backspace
			{
				first=0;
				cp--;
				cx-=8;
				SDLBox(cx,yp,slen,yp+7,146);
				buffer[cp]=0;
				cc=10;
				dd=500;
			} else { // rest chars
				if ((cp<len)&&(cx+8<slen)&&((isalnum(c))||(c=='-')||(c=='_')||(c=='$')||(c=='.')
					||(c=='\\')||(c=='/')||(TILDE_ALLOWED && c=='~')))
				{
					if ((tp==1)&&(!cp)&&(!isdigit(c))&&(c!='$')); // first char neither digit nor $ 
					else if ((tp==1)&&cp&&(buffer[0]!='$')&&(!isdigit(c))) ; // first char digit and adjacent not digit
					else if ((tp==1)&&cp&&(buffer[0]=='$')&&(hex2dec(c)<0)); // first dollar and adjacent not hex
					else if ((tp==2)&&((c=='/')||(c=='\\')||(cp==50))) ;
					else {
						//if (tp==2) c=toupper(c);
						if (tp==1 && buffer[0]=='$') c=toupper(c);
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
			}
		}
		SDLRedraw();
	} while (c!=13);

	SDLVLine(cx,yp,yp+7,146);
	SDLRedraw();

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
 * param default: initial value (optional)
 * returns: the filename
 *==========================================================================*/
char *get_filename(char *title, char * initial)
{
	char *r;
	int py;
	int tmpx;
	int tmpy=EDIT_OFFSET_Y;
	py=64+tmpy;
	tmpx=openDblBufferDialog(DIALOG_CENTER, py, 320, 32, title, 0);
	py+=14;
#if 0
	if (strlen(CONFIG.pathname)>0) {
		SDLstring(tmpx+4,py,CONFIG.pathname);
		py+=10;
	}
#endif
	r=ginput(tmpx+4,py,tmpx+312,38,initial,2);
	closeLastDblBufferDialog();
	return r;
}


int askNoYes(char *msg)
{
	return ask(msg,"*No|*Yes");
	
}

int ask(char *msg, char * answers)
{
	return message_dialog("Question:", msg,answers);
	
}


/*===========================================================================
 * info_dialog
 * display an error dialog
 * param error: the error message
 * returns: nothing useful
 *==========================================================================*/
int info_dialog(char *msg)
{
	return message_dialog("INFO:", msg,NULL);

}
/*===========================================================================
 * error_dialog
 * display an error dialog
 * param error: the error message
 * returns: nothing useful
 *==========================================================================*/
int error_dialog(char *msg)
{
	return message_dialog("ERROR!", msg, NULL);
}
/*===========================================================================
 * message_dialog
 * display an error dialog
 * param title: title of the window
 * param error: the error message
 * param answers: numbered from left to right
 * returns: nothing useful
 *==========================================================================*/
int message_dialog(char * title, char *error, char * answers)
{
	int tmpx;
	int tmpy=EDIT_OFFSET_Y+48;
	
	int width=strlen(error) *8;
	if (width <80) width=80;
	int resval=0;
		
	char answer_shortcuts[10];
	char buf[32];
	
	
	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, width+8, 40, title, 1);
	// SDLstring(tmpx+(width+8-48)/2,68+tmpy,title);
	SDLstring(tmpx+4,78+tmpy,error);
	if (!answers)
	{
		drawbuttonwidth(tmpx+(width-6*8)/2,90+tmpy,"Okay",6,10);
		SDLgetch(1);
	}
	else
	{
		char * c, *co, *a, * ans, ch;
		int i=0;
		memset(buf,0,32);
		strcpy(buf,answers);
		a=answer_shortcuts;
		*a++=27;
	
		ans=buf;
		while (*ans) {
			c=ans;
			while (*c!='|' && *c!='\0') c++;
			*c='\0';
			co=strchr(ans,'*');
			*a++=tolower(*(++co));
			drawbuttonwidth(tmpx+width/4+i*(5*8+10),90+tmpy,ans,5,10);
			ans=++c;
			i++;

		}
		*a++=27;
		*a='\0';
		while (1) {
			ch=SDLgetch(1);
			
			if (a=strchr(answer_shortcuts,ch))
			{
				if (*a) {
					resval=a-answer_shortcuts;
					break;
				}
			}
		}
		
	}
	closeLastDblBufferDialog();
	
	return resval;
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
	char * tmp_name=NULL;
	int tmpx;
	int free_name=1;
	int tmp_step, tmp_base,tmp_write_tp;

	yp=EDIT_OFFSET_Y+64;
	
	tmpx=openDblBufferDialog(DIALOG_CENTER, yp, 320, 98, "Default Options",1);

	yp+=22;
	tmpx+=8;
	SDLstring(tmpx+3,yp,"Current Export format:");
	SDLstring(tmpx+3+23*8,yp,names[options.write_tp]);
	yp+=12;

	SDLstring(tmpx+3,yp,"Select new export format:");
	yp+=8;
	SDLstring(tmpx+11,yp,".BAS .MAE .M65 .ACT .C");
	SDLplotchr(tmpx+19,yp,34,1,dfont);
	SDLplotchr(tmpx+59,yp,45,1,dfont);
	SDLplotchr(tmpx+107,yp,22,1,dfont);
	SDLplotchr(tmpx+139,yp,33,1,dfont);
	SDLplotchr(tmpx+179,yp,35,1,dfont);
	do {
		c=SDLgetch(0);
		if (c==27) goto exit;
		if (c)
			oldname=strchr(hot,toupper(c));
	} while(!oldname);
	c=oldname-hot;
	yp-=8;
	SDLBox(tmpx+3,yp,tmpx+206,yp+16,148);
	SDLstring(tmpx+3,yp,"New export format:");
	SDLstring(tmpx-1+19*8,yp,names[c]);
	tmp_write_tp=c;
	if ((!c)||(c==2)) {
		yp+=10;
		SDLstring(tmpx+11,yp,"Base:");
		oldname=ginput(tmpx+54,yp,tmpx+99,5,NULL,1);
		if (!oldname) goto exit;
		SDLBox(tmpx+53,yp-1,tmpx+100,yp+9,148);
		SDLstring(tmpx+55,yp,oldname);
		tmp_base=num2val(oldname);
		free(oldname);
		yp+=10;
		
		SDLstring(tmpx+11,yp,"Step:");
		oldname=ginput(tmpx+54,yp,tmpx+99,5,NULL,1);
		if (!oldname) goto exit;
		
		SDLBox(tmpx+53,yp-1,tmpx+100,yp+9,148);
		SDLstring(tmpx+55,yp,oldname);
		tmp_step=num2val(oldname);
		free(oldname);
	} else {
		tmp_base=tmp_step=0;
	}
	yp+=12;
#if 0
	SDLstring(tmpx+3,yp,"Base pathname:");
	yp+=12;
	char * path=ginput(tmpx-1,yp,tmpx+305,38,CONFIG.pathname,0);
	
	if (!path) goto exit;
	
	strncpy(CONFIG.pathname,path,PATHNAME_LEN-1);
	free(path);
	CONFIG.pathname[PATHNAME_LEN-1]='\0';
	yp+=20;
#endif
	
	drawbutton(tmpx+97,yp,"Okay?");
	c=SDLgetch(1);
	if (tolower(c)!='o' && tolower(c)!='y' && c!=13) goto exit; 
	
	// Commit dialog values
	free_name=0;
	
	options.step=tmp_step;
	options.base=tmp_base;
	options.write_tp=tmp_write_tp;
	
exit:
	if (free_name && tmp_name) free(tmp_name);
	SDL_ShowCursor(SDL_ENABLE);
	closeLastDblBufferDialog();
	return 1;
}

void handler_config_reset()
{

	CONFIG.checkersLo=144;
	CONFIG.checkersHi=146;
	CONFIG.whiteColor=10;
	CONFIG.screenWidth=512;
	CONFIG.screenHeight=380;
	CONFIG.defaultMapWidth=40;
	CONFIG.defaultMapHeight=24;
	CONFIG.color_display_mode=0;
	CONFIG.maxMapSize=4096*1024;
	CONFIG.pathname[0]='\0';
}

void handler_chmap_reset()
{
	
}

void handler_currentfont_reset()
{
	memcpy(font,dfont,1024);
}

void handler_palette_reset()
{
	memcpy(colortable,colortable_default,3*256);
}

void handler_clut_reset()
{
	memcpy(clut,clut_default,sizeof(clut_default));
}
/*===========================================================================
 * do_options
 * update the user preferences
 * returns: nothing useful
 *==========================================================================*/
int do_defaults()
{
	int c,yp;
	int tmpx;
	
	char * options[]={
		//"Char map?",
		"Current font?",
		"Palette?",
		"CLUT?"
	};
	void (*handlers[])()={
		//handler_chmap_reset,
		handler_currentfont_reset,
		handler_palette_reset,
		handler_clut_reset
	};
	
	int reset[sizeof (options) / sizeof (char *)];
	
	yp=EDIT_OFFSET_Y+64;
	
	tmpx=openDblBufferDialog(DIALOG_CENTER, yp, 200, (sizeof(options)/sizeof( char *))*16+40, "Reset to default:",1);
	yp+=22;
	
	int i;
	
	for (i=0; i<sizeof (options) / sizeof (char *); i++)
	{
		SDLstring(tmpx+3,yp,options[i]);
		SDLstring(tmpx+3+19*8,yp,"(Y/N)");
		SDLplotchr(tmpx+3+20*8,yp,57,1,dfont);
		SDLplotchr(tmpx+3+22*8,yp,46,1,dfont);
		do {
			c=toupper(SDLgetch(0));
		} while ((c!='Y')&&(c!='N')&&(c!=27));
		if (c==27) break;
		
		SDLBox(tmpx+3+19*8,yp,tmpx+3+24*8,yp+8,148);
		SDLstring(tmpx+3+19*8,yp,(c=='Y')?"Yes":"No");
		
		reset[i]=(c=='Y');
		
		yp+=16;
	}
	
	if (c!=27) {
		yp+=2;
		drawbutton(tmpx+59,yp,"Okay");
		c=SDLgetch(1);
	}
	
	SDL_ShowCursor(SDL_ENABLE);
	closeLastDblBufferDialog();
	
	if (c!=27) {
		for (i=0; i<sizeof (options) / sizeof (char *); i++)
			if (reset[i]) handlers[i]();
		
		update_font(bank);
		setdefaultpal();
		draw_edit();
		setprefs();
	}
	
	return 1;
}

int palette_click(int i, int mx, int my)
{
	int lum, col;
	
	if (IN_BOX(mx-EDIT_COLOR_X ,my-EDIT_COLOR_Y,i*44-31,i*44+96,-96,15)) {
		lum=(mx-EDIT_COLOR_X-i*44+31)/16;
		col=(my-EDIT_COLOR_Y+96)/7;
		col=col*16+lum*2;
	}
	else
		col=-1;

	return col;
}

int show_palette(i,x,y)
{
	openDblBufferDialog(x-32,y-130,129,113,"",0);
	SDL_Event event;
	int done=0;
	int col,lum;
	
	setpal();

	for (col=0; col<16; col++)
		for (lum=0; lum<8; lum++)
		{
			SDLBox(x+lum*16+1-32, y+col*7-129, x+lum*16+16-32, y+col*7+6-129, col*16+lum*2);
			if (clut[i]==col*16+lum*2)
				SDLHollowBox(x+lum*16+1-32, y+col*7-129, x+lum*16+16-32, y+col*7+6-129, 14);
			
		}
	
	do {
		SDLRedraw();
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
	setdefaultpal();
	return col>=0;
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
	
	unsigned char tmpclut[9];
	unsigned char c;

	yp=64+EDIT_OFFSET_Y;
	tmpx = openDblBufferDialog(DIALOG_CENTER,yp, 178, 96, "Color Registers", 1);
	yp+=16;
	for(i=0;i<5;i++) {
		SDLHollowBox(tmpx+11,yp,tmpx+21,yp+8,0);
		SDLBox(tmpx+12,yp+1,tmpx+20
			   ,yp+7,clut[i]);
		sprintf(buf,"PF%d:",i);
		SDLstring(tmpx+27,yp,buf);

		sprintf(buf,"%d",clut[i]);
		color=ginput(tmpx+62,yp,tmpx+89,5,buf,1);
		if (!color) goto exit;

		SDLBox(tmpx+61,yp-1,tmpx+100,yp+9,148);
		SDLstring(tmpx+63,yp,color);
		tmpclut[i]=(num2val(color)&254);
		SDLBox(tmpx+12,yp+1,tmpx+20,yp+7,tmpclut[i]);

		free(color);
		yp+=10;
	}
	yp+=10;
	drawbutton(tmpx+49,yp,"Okay");
	c=SDLgetch(1);
	if (c==27) goto exit;
	
	memcpy(clut,tmpclut, sizeof (clut));
	
exit:
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

int select_draw_event_loop(int x, int y, int * dc, int hchar, SDL_Event * evt_in) {
	
	int vchar=256/hchar;
	int yp=0;
	int i, sx,sy;
	SDL_Event event;
	SDL_Event * evt;
	
	evt=NULL;
	i=-1;
	do {
		
		SDLRedraw();
		
		if (!evt_in) {
			SDL_WaitEvent(&event);
			evt=&event;
		} else {
			evt=evt_in;
			yp=1;
		}
		
		switch(evt->type){
			case SDL_KEYDOWN: {
				i=stoa(evt->key.keysym.unicode&0x7f);
				if ((i)||((evt->key.keysym.unicode==' '))) {
					if ((evt->key.keysym.mod&KMOD_ALT))
						i=i+128;
					if (dc) dc[0]=i;
					yp=1;
				}
			}
				break;
			case SDL_MOUSEBUTTONUP: {
				
				if (IN_RANGE(evt->button.button,1,3)) {
					int mx,my;
					
					mx=SDLTranslateClick(evt->button.x);
					my=SDLTranslateClick(evt->button.y);
					// DEBUG PURPOSES JH
			
					if ((mx>=x)&&(mx<=x+hchar*8-1)&&(my>=y)&&(my<=y+vchar*8-1)) {
						sx=(mx-x)/8; sy=(my-y)/8;
						if (0) {
							char buf[128];
							SDLBox(28*8,6,48*8,22,144);
							sprintf(buf,"mxmy  [%05dx%05d]",sx,sy);
							SDLstring(28*8,6,buf);
							sprintf(buf,"scxscy[%05dx%05d]",x,y);
							SDLstring(28*8,14,buf);
						}
						i=sy*hchar+sx;
						if (dc) dc[evt->button.button-1]=i;
						yp=1;
					}
				}
			}
			default:
				
				//if (hchar==16) yp=1;
				break;
		}
	} while (!yp);
	return i;
}
/*===========================================================================
 * select_draw
 * get a draw character from the user
 * param title: title of the dialog box
 * param dc: array of draw colors indexed by mousebutton
 * returns: the selected character
 *==========================================================================*/
int select_draw(int x, int y, char *title, int * dc, int hchar, int process_events)
{
	int i,sx,sy,yp;
	int hb;
	int vb;
	int add;
	hb=8;
	vb=6;
	if (hchar==16) {
		hb=3;
		vb=0;
	}
	
	int vchar=256/hchar;
	add=0;
	if (title) {
		add=10+vb;
	}
	yp=vchar*8+y;
	yp+=add;
	
	raisedbox(x,y,x+hchar*8+2*hb-1,yp+4+vb,2);
	if (title) SDLstring(x+hb,y+hb/2,title);
	
	SDLHLine(x+hb-2,x+hchar*8+hb+1,y+add,144);
	SDLHLine(x+hb-2,x+hchar*8+hb+1,yp+3,152);
	SDLVLine(x+hb-2,y+add+1,yp+3,144);
	SDLVLine(x+hchar*8+hb+1,y+add+1,yp+3,152);
	sx=0; sy=0;
	
	setpal();
	//if (!default_font_mode)
	//	SDLBox(x+hb-1,y+add+1,x+hchar*8+hb,yp+2,clut[0]);
	//else
	SDLBox(x+hb-1,y+add+1,x+hchar*8+hb,yp+2,clut[3]);
	
	for(i=0;i<256;i++) {
		plot_draw_char(x+hb+sx, y+add+2+sy, i);
		sx+=8;
		if (sx>=hchar*8) { sx=0; sy+=8; }
	}
	setdefaultpal();

	if (process_events)	return select_draw_event_loop(x+hb,y+add+2,currentView->dc,hchar,NULL);
	
	return 0;
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
	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 164, 32, title, 0);
	//SDLstring(tmpx+4,68+tmpy,title);

	if (def>=0)
		sprintf(buf,"%d",def);
	else *buf=0;
	do {
		r=ginput(tmpx+4,80+tmpy,tmpx+156,24,buf,1);
		if (r) {
			i=num2val(r);
			free(r);
		} else i=-1;
	} while ((max>=0)&&(i>max));
	closeLastDblBufferDialog();
	return i;
}


unsigned char * resize_map(view * map_view, int x, int y) {
	unsigned char * look, * newmap, *result;
	int loop, i;

	look=newmap=map_view->map;
	if (x<map_view->w) {
		for(loop=0;loop<map_view->h;loop++) {
			for(i=0;i<x;i++)
				*look++=*newmap++;
			newmap+=map_view->w-x;
		}
	}
	newmap=(unsigned char *)realloc(map_view->map,x*y);
	if (newmap) {
		result=newmap;
		if (x>map_view->w) {
			look=result+y*map_view->w;
			newmap=result+y*x;
			for(loop=y-1;loop>=0;loop--)
				for(i=x-1;i>=0;i--)
					if (i<map_view->w) *(--newmap)=*(--look);
					else *(--newmap)=0;
			
		}
		
		if (y>map_view->h)
			memset(result+x*map_view->h,0,x*y-x*map_view->h);
	} else {
		error_dialog("Cannot allocate new map");
		return NULL;
	}
	return result;
}

/*===========================================================================
 * do_size
 * resize a map
 * param tile_mode: flag indicating 0: map mode; 1 tile mode;
 * returns: nothing useful
 *==========================================================================*/
int do_size(int tile_mode)
{
	int x, y, loop, tmax;
	char buf[16], *size;
	int result=0;
	
	if (tile_mode) {
		tile_mode=8; tmax=16;
	} else tmax=65536;

	int tmpx;
	int tmpy=EDIT_OFFSET_Y;
	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 160, 32,/*tile_mode?"Tile width:":"New width:"*/"",0);
	
	if (!tile_mode) {
		SDLstring(tmpx+4,68+tmpy,"New Width:");
		sprintf(buf,"%d",map->w);
	} else {
		SDLstring(tmpx+4,68+tmpy,"Tile Width:");
		sprintf(buf,"%d",tsx);
	}
	size=ginput(tmpx+92+4+tile_mode,68+tmpy,tmpx+92+64-tile_mode,6,buf,1);
	if (size) {
		x=num2val(size);
		free(size);
		if ((x)&&(x<=tmax))
			loop=0;
	} else
		goto exit;
	SDLBox(tmpx+4+91,67+tmpy,tmpx+93+64-tile_mode,77+tmpy,148);
	sprintf(buf,"%d",x);
	SDLstring(tmpx+4+93+tile_mode,68+tmpy,buf);
	
	if (!tile_mode) {
		SDLstring(tmpx+4,78+tmpy,"New Height:");
		sprintf(buf,"%d",map->h);
	} else {
		SDLstring(tmpx+4,78+tmpy,"Tile Height:");
		sprintf(buf,"%d",tsy);
	}

	size=ginput(tmpx+4+92+tile_mode,78+tmpy,tmpx+92+64-tile_mode,6,buf,1);
		if (size) {
			y=num2val(size);
			free(size);
			if ((y)&&(y<=tmax))
				loop=0;
		} else goto exit;
	
	// operations - to move to another place (JH)
	
	if (tile_mode) {
		tile_size(x,y);
		goto exit;
	}
	if ((map->w==x)&&(map->h==y)) goto exit;
	
	unsigned char * result_map=resize_map(map, x, y);
	unsigned char * result_mask=resize_map(mask, x, y);
	
	if (result_map && result_mask)
	{
		map->map=result_map;
		mask->map=result_mask;
		map->w=mask->w=x;
		map->h=mask->h=y;
		result=1;
		
	} else {
		if (!result_map)  error_dialog("Failed to realloc map");
		if (!result_mask)  error_dialog("Failed to realloc mask");
		// possible bug here - map was reallocated and mask not.
		// inconsistency between mask and map
	}
		
	
	map->cx=map->cy=map->scx=map->scy=0;
	
exit:
	closeLastDblBufferDialog();

	return result;
}

/*===========================================================================
 * do_move
 * jump to a screen position
 * param map: the map to update
 * param tile_mode: flag indicating 0: map mode; 1 tile mode;
 * returns: nothing useful
 *==========================================================================*/
int do_move(view *map, int tile_mode)
{
	int mw, mh, x, y, loop;
	char buf[16], *size;

	int tmpx;
	int tmpy=EDIT_OFFSET_Y;

	tmpx=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 168, 32,"",0);
	
	if (tile_mode) {

		SDLstring(tmpx+4,68+tmpy,"Go to tile:");
		int tx, ty;
		tx=((currentView->scx+currentView->cx)/tsx);
		ty=((currentView->scy+currentView->cy)/tsy);
		sprintf(buf,"%d",tx+ty*16);
		loop=1;
		do {
			size=ginput(tmpx+100,68+tmpy,tmpx+164,6,buf,1);
			if (!size) goto exit;
			x=num2val(size);
			free(size);
			if (IN_RANGE(x,0,255))
				loop=0;
		} while(loop);
		y=x/16; x=x-y*16;
		x=x*tsx; y=y*tsy;
	} else {
		SDLstring(tmpx+4,68+tmpy,"Move to X:");
		sprintf(buf,"%d",map->scx+map->cx);
		loop=1;
		do {
			size=ginput(tmpx+92,68+tmpy,tmpx+92+64,6,buf,1);
			if (!size) goto exit;
			x=num2val(size);
			free(size);
			if (IN_RANGE(x,0,map->w-1)) loop=0;
				else sprintf(buf,"%d",map->w-1);
		} while (loop);
		
		SDLBox(tmpx+91,67+tmpy,tmpx+92+65,77+tmpy,148);
		sprintf(buf,"%d",x);
		SDLstring(tmpx+93,68+tmpy,buf);

		SDLstring(tmpx+4,78+tmpy,"Move to Y:");
		sprintf(buf,"%d",map->scy+map->cy);
		loop=1;
		do {
			size=ginput(tmpx+92,78+tmpy,tmpx+92+64,6,buf,1);
			if (!size) goto exit;			
			y=num2val(size);
			free(size);
			if (IN_RANGE(y,0,map->h-1)) loop=0;
			else sprintf(buf,"%d",map->h-1);
		} while (loop);
		
	}

	mw=CONFIG.screenWidth/map->cw;
	mh=(CONFIG.screenHeight-24)/map->ch;
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

exit:
	closeLastDblBufferDialog();

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
	x=openDblBufferDialog(DIALOG_CENTER, 64+tmpy, 164, 32,"Warning!",1);
	SDLstring(x+4+16,78+tmpy,"Really Exit? Y/N");
	SDLplotchr(x+4+128+8,78+tmpy,stoa('N'),1,dfont);
	setprefs();
	
	ch=SDLgetch(1);
	closeLastDblBufferDialog();
	return ((ch=='Y')||(ch=='y'));

}
/*=========================================================================*/
