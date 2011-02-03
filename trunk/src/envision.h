/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: envison.h                                                        *
 * Author: Mark Schmelzenbach                                             *
 * Started: 07/29/97                                                      *
 =========================================================================*/
#include <stdio.h>

typedef struct opt {
  char *disk_image;
  int write_tp;
  int base, step;
} opt;

typedef struct view {
  int w, h;     /* map width, height */
  int scx, scy; /* Upper left corner of screen */
  int cx, cy;   /* Cursor offset from upper left */
  int cw, ch;   /* Cursor width, height */
  int dc;       /* draw color */
  unsigned char *map;
} view;

typedef struct config
	{
		int checkersHi;
		int checkersLo;
		int whiteColor;
	} config;

#define IN_RANGE(x,lx,rx) ((x)>=(lx)&&(x)<=(rx))
#define IN_BOX(x,y,lx,rx,ly,ry) (IN_RANGE((x),(lx),(rx))&&IN_RANGE((y),(ly),(ry)))

extern config CONFIG;

int title();

int get_8x8_mode(int m);
int do_map();
int do_colors();
int do_size(int tmode);
int do_exit();
int do_move(view *map, int tmode);
int select_draw(char *title);
int error_dialog(char *error);
char stoa(char s);
int tile_size(int w, int h);

int do_mode(int m);
int draw_screen(int b);
int draw_header(int update);

void bye();
void txterr(char *txt);
int drawbutton(int x, int y, char *txt);
char *get_filename(char *title, char *image);
int get_number(char *title, int def, int max);
int select_char(char *title);
int do_options();
int command(int cmd, int sym);

int read_xfd_font(char *image, char *file, unsigned char *data, int max);
int write_xfd_font(char *image, char *file, unsigned char *data, int max, FILE *in);
int write_xfd_data(char *image, char *file, unsigned char *data, int start, int end);

int read_font(char *file, unsigned char *font);
int write_font(char *file, unsigned char *font);
int write_data(char *file, unsigned char *font, int start, int end, int a);

int write_map(char *file, unsigned char *font, view *map, int raw);
view *read_map(char *file, unsigned char *font, view *map);
view *read_xfd_map(char *image, char *file, unsigned char *font, view *map);
int write_xfd_map(char *image, char *file, unsigned char *font, view *map, int raw);

int tile_map(int w, int h, view *map, view *tile);
int untile_map(view *map, view *tile);

extern view *currentView, *map, *tile;
extern unsigned char *dfont, *font, *cache;
extern int echr, cacheOk;
extern opt options;
extern unsigned char clut[9];
extern int cmds[13];
extern int mode, ratio;
extern int base;
extern int tsx, tsy, tmode;
extern unsigned long s1,s2;
