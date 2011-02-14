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

typedef struct runtime_storage
	{
		char save_map_file_name[128];
		char save_raw_map_file_name[128];
		char save_raw_mask_file_name[128];
		char save_font_file_name[128];
		char export_font_file_name[128];
	} runtime_storage;

typedef enum {COLOR_DISPLAY_HEX, COLOR_DISPLAY_DEC, COLOR_DISPLAY_MAX} color_display_type;

typedef struct config
	{
		int checkersHi;
		int checkersLo;
		int whiteColor;
		int screenHeight;
		int screenWidth;
		int defaultMapWidth;
		int defaultMapHeight;
		color_display_type color_display_mode;
	} config;

typedef struct rgb_color {
	unsigned char r,g,b;
} rgb_color;// __attribute__((__packed__));

#define IN_RANGE(x,lx,rx) ((x)>=(lx)&&(x)<=(rx))
#define IN_BOX(x,y,lx,rx,ly,ry) (IN_RANGE((x),(lx),(rx))&&IN_RANGE((y),(ly),(ry)))

#define MAP_TOP_OFFSET	24
#define MAX_DIALOG_WIDTH 320
#define MAX_DIALOG_HEIGHT 128

#define EDIT_OFFSET_X 64
#define EDIT_OFFSET_Y 64

#define EDIT_GRID_X (272+EDIT_OFFSET_X)
#define EDIT_GRID_Y (32+EDIT_OFFSET_Y)

#define EDIT_FONTSEL_X (344+EDIT_OFFSET_X)
#define EDIT_FONTSEL_Y (176+EDIT_OFFSET_Y)
#define EDIT_MODESEL_X (344+EDIT_OFFSET_X)
#define EDIT_MODESEL_Y (194+EDIT_OFFSET_Y)

#define EDIT_CHARMAP_X (120+EDIT_OFFSET_X)
#define EDIT_CHARMAP_Y (136+EDIT_OFFSET_Y)

#define EDIT_CORNER_X (144+EDIT_OFFSET_X)
#define EDIT_CORNER_Y (32+EDIT_OFFSET_Y)

#define EDIT_MENU_X (EDIT_OFFSET_X)
#define EDIT_MENU_Y (24+EDIT_OFFSET_Y)

#define EDIT_COLOR_X (118+EDIT_OFFSET_X)
#define EDIT_COLOR_Y (176+EDIT_OFFSET_Y)

#define BUTTON_WIDTH 12

#define TILE_MODE ((!tileEditMode)&&((tsx>1)||(tsy>1)))
#define NOT_TILE_MODE (!TILE_MODE)

#define MASK_EDIT_MODE (maskEditMode)
#define NOT_MASK_EDIT_MODE (!maskEditMode)

enum {DIALOG_LEFT=-3, DIALOG_CENTER, DIALOG_RIGHT};
enum {VIEW_MAP, VIEW_TILE, VIEW_MASK};
enum {MAP_NO_ALLOC,MAP_ALLOC};
enum {FILE_NATIVE,FILE_RAWMAP,FILE_RAWMASK};


extern config CONFIG;
extern int MAP_MENU_HEIGHT;
extern int bank;
extern rgb_color colortable[256];
extern runtime_storage RUNTIME_STORAGE;
extern char commands_allowed[64];

int title();

//dialog.c
int get_8x8_mode(int m);
int do_colors();
int do_size(int tile_mode);
int do_exit();
int do_move(view *map, int tile_mode);
int select_draw(char *title);
int error_dialog(char *error);
int info_dialog(char *error);
int message_dialog(char * title, char *error);
char stoa(char s);
int raisedbox(int x, int y, int w, int h);
int do_defaults();
unsigned char * resize_map(view * map_view, int x, int y);
int drawbutton(int x, int y, char *txt);
char *get_filename(char *title, char *image, char * initial);
int get_number(char *title, int def, int max);
int select_char(char *title);
int do_options();
int show_palette(int i,int x, int y);

// handlers for "reset" dialog
void handler_config_reset();
void handler_chmap_reset();
void handler_currentfont_reset();
void handler_palette_reset();
void handler_clut_reset();

//map.c
int do_map();
int do_mode(int m);
int draw_screen(int b);
int draw_header(int update);
void plot_draw_char(int x, int y, unsigned char c);
int tile_size(int w, int h);



// envision.c
void txterr(char *txt);
void setpal();
void setdefaultpal();
void set_allowed_commands(int * cmds, int cmdcnt);
int is_command_allowed(int cmd);
void bye();
int command(int cmd, int sym);
int hex2dec(char hex);
int num2val(char * chrval);
int draw_edit();
int update_font(int b);
void draw_numbers(int vals, unsigned char *work);
view * map_init(int alloc_map, view * map, int width, int height);






// fileio.c
int read_xfd_font(char *image, char *file, unsigned char *data, int max);
int write_xfd_font(char *image, char *file, unsigned char *data, int max, FILE *in);
int write_xfd_data(char *image, char *file, unsigned char *data, int start, int end);

int read_font(char *file, unsigned char *font);
int write_font(char *file, unsigned char *font);
int write_data(char *file, unsigned char *font, int start, int end, int a);

int write_map(char *image, char *file, unsigned char *font, view *map, int raw);
int write_file_map(char *file, unsigned char *font, view *map, int raw);
int write_xfd_map(char *image, char *file, unsigned char *font, view *map, int raw);
view *read_map(char *image, char *file, unsigned char *font, view *map, int raw);
view *read_file_map(char *file, unsigned char *font, view *map, int raw);
view *read_xfd_map(char *image, char *file, unsigned char *font, view *map, int raw);
long flength(FILE * fd);
int import_palette(char *file, rgb_color * colortable);

// util.c
int tile_map(int w, int h, view *map, view *tile);
int untile_map(view *map, view *tile);

int unpack(unsigned char *look, int xs, int ys);


extern view *currentView, *map, *tile, *mask;
extern unsigned char *dfont, *font, *cache;
extern int echr, cacheOk;
extern opt options;
extern unsigned char clut[9];
extern unsigned char clut_default[9];
extern int cmds[32];
extern int mode, ratio;
extern int base;
extern int tsx, tsy, tileEditMode;
extern int maskEditMode;
extern unsigned long s1,s2;
