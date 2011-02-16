/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: fileio.c                                                         *
 * Author: Mark Schmelzenbach                                             *
 * Started: 07/28/97                                                      *
 * Long overdue port to SDL 1/26/2006                                     *
 =========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "envision.h"

#define dsksize 720
#define secsize 128
#define lowb(x)     ((x)&0x00ff)
#define highb(x)    (((x)>>8)&0x00ff)

unsigned char secbuf[256];
unsigned char VTOCsec[128];

long flength(FILE * in)
{
	long len;
	fseek(in,0L,SEEK_END);
	len=ftell(in);
	rewind(in);
	return len;
}

FILE * openFile(char * file, char * access, char * failed) {
	FILE * fd;
	fd=fopen(file,access);
	if (!fd) {
		if (failed) error_dialog(failed);
		return 0;
	}
	return fd;
}
/*=========================================================================*/

int file_exists(char * fname) {
	FILE * fd;
	if(!(fd=openFile(fname,"rb",NULL))) return 0;
	fclose(fd);
	return 1;
}


int overwrite( char * fname)
{
	int res=1;
	if (file_exists(fname)) {
		res=askNoYes("File exists. Overwrite?");
		return res==2;
	}
	return res;
}

/*=========================================================================*/
int import_palette(char *file, rgb_color * colortable)
{
	FILE *in;
	long len;
	unsigned char ct[768];
		
	if(!(in=openFile(file,"rb","Cannot open file"))) return -1;
	
	if (flength(in)!=768) {
		error_dialog("Incorrect palette file (length<>768)");
		fclose(in);
	} else {
		len=fread(ct,1,768,in);
		if (len!=768) 
		{
			error_dialog("Could not read.");
			fclose(in);
			return 0;
		}
		else
			memcpy(colortable, ct, 768);
	}
	fclose(in);
	return 1;
}
/*=========================================================================*/
int read_font(char *file, unsigned char *font)
{
	FILE *in;
	if(!(in=openFile(file,"rb","Cannot open font!"))) return -1;

	fread(font,1024,1,in);
	
	fclose(in);
	return 0;
}
/*=========================================================================*/
int write_font(char *file, unsigned char *font)
{
  FILE *out;

	if(!(out=openFile(file,"wb","Cannot save font!"))) return -1;

	fwrite(font,1024,1,out);
	fclose(out);
	return 0;
}
/*=========================================================================*/
int write_data(char *file, unsigned char *font, int start, int end, int a)
{
	// first line number or null, next line_number,first line, line_beg, data_fmt, data_sep, eol_sep, last_line
	char *export_formats[5][8]={
		{NULL, "%d",NULL,                      " DATA ",  "%d",    ",", "%c", "%c"},
		{"",   "",  "FONT%c",                  "%c.BY ",  "%d",    ",", "%c", "%c"},
		{"%d ","%d","FONT%c",                  "%c.BYTE ","%d",    ",", "%c", "%c"},
		{"",   "",  "byte array FONT=[%c",     "%c",      "%d",    " ", "%c", "]%c"},
		{"",   "",  "unsigned char FONT[]={%c","%c",      "0x%02x",",", ",%c","};%c"},
	};
	
	FILE *out;
	int i,j,c,line;
	unsigned char tb,cr;
	
	if (a) {
		tb=0x7f;
		cr=155;
	} else {
		tb='\t';
		cr='\n';
	}
	
	if(!(out=openFile(file,"wb","Cannot export font!"))) return -1;
	line=options.base;
	
	if (export_formats[options.write_tp][0])
	{
		// first line, line number
		fprintf(out,export_formats[options.write_tp][0],line);
		line+=options.step;
		
		// first line, label
		fprintf(out,export_formats[options.write_tp][2],cr);
	}
	
	for(i=start;i<=end;i++) {
		
		// data lines, line number
		fprintf(out,export_formats[options.write_tp][1],line);
		line+=options.step;
		
		// data statement
		fprintf(out,export_formats[options.write_tp][3],tb);
		
		for(j=0;j<8;j++) {
			
			c=*font++;
			
			// data_member 
			fprintf(out,export_formats[options.write_tp][4],c);
	
			if (j<7)
				// in-data separator
				fprintf(out,export_formats[options.write_tp][5]);
			else
				// eol separator
				fprintf(out,export_formats[options.write_tp][6],cr);
		}
		
	}	
	// eof
	fprintf(out,export_formats[options.write_tp][7],cr);
	
	
	fclose(out);
	return 0;
}


/*=========================================================================*/
view *read_file_map(char *file, unsigned char *font, view *map, int raw)
{
	FILE *in;
	unsigned char head[16];
	int tp,i;
	int tile_inited=0;
	if (!(in=openFile(file,"rb","Cannot read map!"))) return NULL;

	if (!raw) {
		fread(head,10,1,in);
		mode=head[0];

		map=map_init(MAP_ALLOC, map, head[1]+head[2]*256, head[3]+head[4]*256);
		mask=map_init(MAP_ALLOC, mask, head[1]+head[2]*256, head[3]+head[4]*256);
		memcpy(clut,&head[5],5);
	
	}
	
	if (!raw || raw==FILE_RAWMASK)
		memset(mask->map,0,mask->w*mask->h);
	
	
	
	if (!raw || raw==FILE_RAWMAP)
	
		if (!raw || raw==FILE_RAWMAP) {
			memset(map->map,0,map->w*map->h);
			fread(map->map,map->w*map->h,1,in);
		}
	
	if (raw==FILE_RAWMASK) {
		fread(mask->map,mask->w*mask->h,1,in);
	}
	
	if (raw==FILE_RAWBITMASK) {
		unsigned char * look=mask->map;
		unsigned char ch=0;
		int i;
		for (i=0; i<mask->w*mask->h; i++)
		{
			if ((i&7)==0) ch=fgetc(in);
			*look++=!!(ch&1<<(7-(i&0x7)));
		}
		
	}
	
	if (!raw) {
		fread(font,1024,1,in);
		
		tsx=tsy=1;
		//default tiles
		tile=map_init(MAP_NO_ALLOC,tile, 16, 16);
		while (!feof(in)) {
			tp=fgetc(in);
			if (tp!=EOF) {
				switch(tp) {
					case 1: {
						int i,num;
						unsigned char *look;
						
						fread(head,6,1,in);
						tsx=head[1]*256+head[0];
						tsy=head[3]*256+head[2];
						num=head[5]*256+head[4]+1;
						
						tile=map_init(MAP_ALLOC,tile,16*tsx, 16*tsy);
						tile_inited=1;
						
						for(i=0;i<num;i++) {
							int tx,ty,w,h;
							ty=i/16;
							tx=i%16;
							look=tile->map+tx*tsx+ty*tsy*tsx*16;
							for(h=0;h<tsy;h++) {
								for(w=0;w<tsx;w++) {
									*look=fgetc(in);
									look++;
								}
								look+=tile->w-tsx;
							}	  
						}
					}
						break;
					case 2: {
						/* mask map corresponds to map->map
						 * It has the same dimensions
						 * so it is not nessesary to put them in the file
						 * I see no particular reason to have map of different size than main map.
						 */
						
						fread(mask->map,mask->w*mask->h,1,in);
						break;
					default:
						break;
						
					}
				}
			}
		}
		if (!tile_inited) {
			tile=map_init(MAP_ALLOC,tile, 16, 16);
			for(i=0;i<256;i++)
				tile->map[i]=i;
		}
	}
	fclose(in);
	return map;
}

int write_map(char *file, unsigned char *font, view *map, int raw)
{
	return write_file_map(file,font,map,raw);
}

view *read_map(char *file, unsigned char *font, view *map, int raw)
{
	return read_file_map(file,font,map,raw);
}


/*=========================================================================*/
int write_file_map(char *file, unsigned char *font, view *map, int raw)
{
	FILE *out;
	unsigned char head[16];

	if(!(out=openFile(file,"wb","Cannot save map!"))) return -1;

	if (!raw) {
		head[0]=mode;
		head[1]=map->w&255;
		head[2]=(map->w>>8)&255;
		head[3]=map->h&255;
		head[4]=(map->h>>8)&255;
		head[5]=clut[0];
		head[6]=clut[1];
		head[7]=clut[2];
		head[8]=clut[3];
		head[9]=clut[4];
		fwrite(head,10,1,out);
	}
	if (!raw || raw==FILE_RAWMAP)
		fwrite(map->map,map->w*map->h,1,out);
	
	if (raw==FILE_RAWMASK)
		fwrite(mask->map,mask->w*mask->h,1,out);
	
	if (raw==FILE_RAWBITMASK) {
		unsigned char * look=mask->map;
		unsigned char ch=0;
		int i;
		for (i=0; i<mask->w*mask->h; i++)
		{
			if ((i&7)==0) ch=0;
			if (*look++) ch|=1<<(7-(i&0x7));
			if ((i&7)==7) fputc(ch, out);
		}
		if ((mask->h*mask->w)&0x7) fputc(ch, out);
	}
	
	if (!raw) {
		fwrite(font,1024,1,out);
		if ((tsx>1)||(tsy>1)) {
			int i;
			unsigned char *look;
			
			fputc(1,out); /* signal tilemap type 1 block */
			head[0]=tsx&255;
			head[1]=(tsx>>8)&255;
			head[2]=tsy&255;
			head[3]=(tsy>>8)&255;
			head[4]=255;  /* num tiles -1 */
			head[5]=0;
			fwrite(head,6,1,out);
			
			for(i=0;i<256;i++) {
				int tx,ty,w,h;
				ty=i/16;
				tx=i%16;
				look=tile->map+tx*tsx+ty*tsy*tsx*16;
				for(h=0;h<tsy;h++) {
					for(w=0;w<tsx;w++) {
						fputc(*look,out);
						look++;
					}
					look+=tile->w-tsx;
				}
			}
		}
		/* mask map corresponds to map->map
		 * It has the same dimensions
		 * so it is not nessesary to put them in the file
		 * I see no particular reason to have map of different size than main map.
		 */
		fputc(2,out); /* signal tilemap type 1 block */		
		fwrite(mask->map,mask->w*mask->h,1,out);
		
		fputc(0,out); /* signal end of blocks */
		
	}
	fclose(out);
	return 0;
}
/*=========================================================================*/
