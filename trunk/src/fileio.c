/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: fileio.c                                                         *
 * Author: Mark Schmelzenbach                                             *
 * Started: 07/28/97                                                      *
 * Long overdue port to SDL 1/26/2006                                     *
 =========================================================================*
 * xfd disk routines based on xfd_tools package by Ivo van Poorten (P)1995*
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

static unsigned char BOOT_sectors[] ={
0xc4,0x03,0x00,0x07,0xfb,0x12,0x38,0x60,0x28,0x04,0x00,0x04,0x00,0x1e,0x80,0x01,
0x02,0x00,0x04,0x05,0x06,0x07,0x00,0x3b,0x9b,0x4e,0x55,0x9b,0x45,0x4e,0x55,0x9b,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x9b,0xa9,0x80,0xa0,0x07,0x20,0x63,0x07,0xa9,0x7d,0x85,0x45,0xa0,0x04,0xa9,0x00,
0x20,0x71,0x07,0x30,0x15,0x20,0x5c,0x07,0xa0,0x00,0xb1,0x43,0x29,0x03,0xaa,0xc8,
0x11,0x43,0xf0,0x07,0xb1,0x43,0xa8,0x8a,0x10,0xe6,0x38,0x60,0x18,0xa4,0x44,0xa5,
0x43,0x65,0x45,0x85,0x43,0x8d,0x04,0x03,0x98,0x69,0x00,0x85,0x44,0x8d,0x05,0x03,
0x60,0x8d,0x0b,0x03,0xa2,0x52,0x90,0x02,0xa2,0x50,0x8c,0x0a,0x03,0xa0,0x03,0x98,
0x8e,0x02,0x03,0x8d,0x06,0x03,0x8c,0x68,0x1b,0xa9,0x31,0x8d,0x00,0x03,0xa9,0x80,
0xa0,0x00,0x24,0x45,0x10,0x02,0x0a,0xc8,0x8d,0x08,0x03,0x8c,0x09,0x03,0xa9,0x40,
0xae,0x02,0x03,0xec,0x79,0x07,0xd0,0x01,0x0a,0x8d,0x03,0x03,0xad,0x01,0x03,0xf0,
0x62,0x20,0x59,0xe4,0x10,0x14,0xad,0x0f,0xd2,0x29,0x08,0xf0,0x05,0xce,0x68,0x1b,
0xd0,0xdc,0xad,0x5d,0x0e,0xf0,0x05,0x4c,0x5d,0x0e,0xa6,0x18,0x98,0x60,0x99,0x08,
0xa6,0x0a,0x71,0x0a,0xc2,0x09,0x1b,0x0b,0xfe,0x0a,0x44,0xce,0x07,0x4d,0xce,0x07,
0xa9,0x00,0x8d,0x44,0x02,0xa8,0x99,0x03,0x1b,0xc8,0xd0,0xfa,0xad,0x09,0x07,0xf0,
0x04,0xc9,0x0b,0x90,0x05,0xa9,0x04,0x8d,0x09,0x07,0xa8,0x99,0x75,0x1b,0x20,0x6a,
0x12,0x8d,0xe8,0x02,0x8c,0xe7,0x02,0xa0,0x05,0xb9,0xda,0x07,0x99,0x29,0x03,0x88,
0x10,0xf7,0x60,0xa9,0x00,0x85,0x30,0xae,0x01,0xd3,0x18,0xac,0x0a,0x03,0xad,0x0b,
0x03,0x2c,0x0e,0x07,0x70,0x1b,0x30,0x02,0x69,0x04,0x85,0x32,0x98,0x0a,0x26,0x32,
0x38,0x6a,0x4a,0x85,0x31,0x66,0x30,0xa4,0x32,0x8a,0x09,0xfc,0x39,0x8e,0x08,0xd0,
0x18,0x98,0xac,0x0b,0x03,0xf0,0x02,0xe9,0x67,0xc9,0x20,0x90,0x02,0x69,0x0f,0x4a,
0x66,0x30,0x69,0xc0,0x85,0x31,0x8a,0x29,0xfe,0x78,0xa0,0x00,0x8c,0x0e,0xd4,0x8d,
0x01,0xd3,0xad,0x04,0x03,0x85,0x32,0xad,0x05,0x03,0x85,0x33,0xad,0x03,0x03,0x10,
0x08,0xb1,0x32,0x91,0x30,0xc8,0x10,0xf9,0x88,0xb1,0x30,0x91,0x32,0xc8,0x10,0xf9
};

// 360 sector
static unsigned char VTOC_sectors[] ={
0x02,0xc3,0x02,0xc3,0x02,0x00,0x00,0x00,0x00,0x00,0x0f,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

long flength(FILE * in)
{
	long len;
	fseek(in,0L,SEEK_END);
	len=ftell(in);
	rewind(in);
	return len;
}


/*=========================================================================*/
int is_xfd(FILE *image)
{
  if (flength(image)==92160) return 1;
	else return 0;
}


int xfd_file_exists(char * fname) {
	FILE * xfd;
	xfd=fopen(fname,"rb");
	if (!xfd) {
		return -1;
	}
	if (!is_xfd(xfd)) {
		fclose(xfd);
		return -2;
	}
	fclose(xfd);
	return 0;
}

int file_exists(char * fname)
{
	int res=xfd_file_exists(fname);
	// if not exist
	if (res==-1) return 0;
	return 1;
}

/*=========================================================================*/
void convertfname(char *in, char *out)
{
	int x,y;
	
	for(x=0; x<11; x++)
		out[x]=32;
	out[11]=0;
	
	x=0;
	y=*in++;
	
	while ((y!=0)&&(y!='.')) {
		out[x]=y;
		if(x!=8) x++;
		y=*in++;
	}
	out[8]=32;
	if (y!=0) {
		x=8;
		y=*in++;
		while((x<11)&&(y)&&(y!='.')) {
			out[x]=y;
			x++;
			y=*in++;
		}
	}
}
/*=========================================================================*/
int readsec(FILE *image, int nr)
{
	if ((nr>dsksize)||(nr<1))
		return 0;
	fseek(image,(long)(nr-1)*secsize,SEEK_SET);
	fread(secbuf,secsize,1,image);
	return 1;
}

/*=========================================================================*/
int scandir(FILE *image, char *filename)
{
	int secnum,cnt,status,length,startsec;
	int endofdir;
	char fname[12];
	
	endofdir=0;
	secnum=361;
	startsec=-1;
	
	while(!endofdir) {
		readsec(image,secnum);
		
		for(cnt=0; cnt<8; cnt++) {
			status=secbuf[cnt*16];
			length=secbuf[cnt*16+1]+256*secbuf[cnt*16+2];
			
			if (!status) {
				endofdir=1;
				break;
			}
			
			if (!(status&0x80)) {
				memcpy(fname,&secbuf[cnt*16+5],11);
				fname[11]=0;
				if (strncmp(filename,fname,11)==0)
					startsec=secbuf[cnt*16+3]+secbuf[cnt*16+4]*256;
			}
		}
		
		secnum++;
		if (secnum>368)
			endofdir=1;
	}
	return startsec;
}

int xfd_image_file_exists(char *image, char *file)
{
	FILE *xfd;
	int startsec;
	char fname[12];
	
	xfd=fopen(image,"rb");
	if (!xfd) {
		error_dialog("Cannot open image");
		return -1;
	}
	if (!is_xfd(xfd)) {
		error_dialog("Not an .XFD image");
		return -2;
	}
	
	convertfname(file,fname);
	
	startsec=scandir(xfd,fname);
	if (startsec<0)  {
		return -3;
	}
	return 0;
}	

int overwrite( char * fname)
{
	int res=1;
	if (options.disk_image) {
		if (!xfd_image_file_exists(options.disk_image,fname)) {
			info_dialog("File on image exists. Can not delete.");
			return 0;
		}
	}
	else {
		if (file_exists(fname)) {
			res=askNoYes("File exists. Overwrite?");
			return res==2;
		}
	}
	return res;
}

int xfd_format_if_needed() {
	int skip=0;
	if (options.disk_image) {
		switch (xfd_file_exists(options.disk_image)) {
			case -1: 
				if (2==askNoYes("Image file does not exist. Create?"))
					xfd_file_format(options.disk_image);
				else skip=1;
				break;
			case -2: 
				info_dialog("File exists but has incorrect length. Aborting.");
				skip=1;
				break;
			default:
				// ok, file exists and has the right length
				break;								
		}
	}
	return skip;
}

int xfd_file_format (char * fname)
{
	FILE * xfd;
	xfd=fopen(fname,"wb");
	if (!xfd) {
		return -1;
	}
	xfd_format(xfd);
	fclose(xfd);
	return 0;
}


int xfd_format(FILE * image)
{
	unsigned char buffer[secsize];
	int i;
	memset(buffer, 0, secsize);
	fseek(image,0L,SEEK_SET);
	fwrite(BOOT_sectors, 128, 3, image);
	
	for (i=4; i<=720; i++)
		fwrite(buffer, 128, 1, image);
	
	fseek(image,359*secsize,SEEK_SET);
	fwrite(VTOC_sectors, 128, 1, image);
	return 0;
}


/*=========================================================================*/
int writesec(FILE *image, int nr)
{
  if ((nr>dsksize)||(nr<1))
    return 0;

  fseek(image,(long)(nr-1)*secsize,SEEK_SET);
  fwrite(secbuf,secsize,1,image);
  return 1;
}
/*=========================================================================*/
void readVTOC(FILE *image)
{
  fseek(image,(long)(360-1)*secsize,SEEK_SET);
  fread(VTOCsec,secsize,1,image);
}
/*=========================================================================*/
void writeVTOC(FILE *image)
{
  fseek(image,(long)(360-1)*secsize,SEEK_SET);
  fwrite(VTOCsec,secsize,1,image);
}
/*=========================================================================*/
int find_free_sec(FILE *image)
{
  int x,y;
  int freesec;

  readVTOC(image);

  for(x=10; x<100; x++)
    if (VTOCsec[x]!=0) break;

  freesec=(x-10)*8;

  y=VTOCsec[x];

  if (y&0x80) freesec+=0;
  else if (y&0x40) freesec+=1;
  else if (y&0x20) freesec+=2;
  else if (y&0x10) freesec+=3;
  else if (y&0x08) freesec+=4;
  else if (y&0x04) freesec+=5;
  else if (y&0x02) freesec+=6;
  else if (y&0x01) freesec+=7;

  if (y>=720) y=0;

  return freesec;
}
/*=========================================================================*/
void marksec(FILE *image, int nr, int *secsfree)
{
  int byte,bit;

  readVTOC(image);

  byte=nr/8;
  bit=nr%8;

  VTOCsec[10+byte]&=((0x80>>bit)^0xff);

  *secsfree=*secsfree-1;
  VTOCsec[3]=lowb(*secsfree);
  VTOCsec[4]=highb(*secsfree);

  writeVTOC(image);
}
/*=========================================================================*/
void writedirentry(FILE *image, char *file, int startsec, int len, int entry)
{
  int qwe,asd;
  int x;

  qwe=entry/8;
  asd=entry%8;

  readsec(image,361+qwe);

  secbuf[asd*16+0]=0x42;
  secbuf[asd*16+1]=lowb(len/125+1);
  secbuf[asd*16+2]=highb(len/125+1);
  secbuf[asd*16+3]=lowb(startsec);
  secbuf[asd*16+4]=highb(startsec);

  for(x=0;x<12;x++)
    secbuf[asd*16+5+x]=file[x];

  writesec(image,361+qwe);
}
/*=========================================================================*/
int find_newentry(FILE *image) {
  int secnum,cnt,status,entrynum;
  int endofdir;

  endofdir=0;
  secnum=361;
  entrynum=-1;

  while(!endofdir) {
    readsec(image,secnum);

    for(cnt=0;cnt<8;cnt++) {
      status=secbuf[cnt*16];

      if ((status==0)||(status&0x80)) {
	entrynum=(secnum-361)*8+cnt;
	endofdir=1;
	break;
      }

    }
    secnum++;
    if (secnum>368) endofdir=1;
  }
  return entrynum;
}
/*=========================================================================*/
int read_xfd_font(char *image, char *file, unsigned char *data, int max)
{
  FILE *xfd;
  int count, next, startsec, len;
  char fname[12];

  xfd=fopen(image,"rb");
  if (!xfd) {
    error_dialog("Cannot open image");
    return -1;
  }
  if (!is_xfd(xfd)) {
    error_dialog("Not an .XFD image");
    return -2;
  }

  convertfname(file,fname);

  startsec=scandir(xfd,fname);
  if (startsec<0)  {
    error_dialog("File not found");
    return -3;
  }

  len=0;
  readsec(xfd,startsec);
  count=secbuf[127];
  next=secbuf[126]+(secbuf[125]&0x03)*256;

  while (1) {
    memcpy(data+len,secbuf,count);
    len=len+count;

    if (next==0) break;

    readsec(xfd,next);
    count=secbuf[127];
    next=secbuf[126]+(secbuf[125]&0x03)*256;
    if (len+count>max) {
      fclose(xfd);
      return 0;
      /*   error_dialog("Load buffer full");
	   return -4; */
    }
  }
  fclose(xfd);
  return 1;
}
/*=========================================================================*/
int write_xfd_font(char *image, char *file, unsigned char *data, int max, FILE *in)
{
  FILE *xfd;
  int startsec,cursec,nextsec;
  int entrynum, secsfree;
  int qwe,x;
  char fname[12];

  xfd=fopen(image,"rb+");
  if (!xfd) {
    error_dialog("Cannot open image");
    return -1;
  }
  if (!is_xfd(xfd)) {
    error_dialog("Not an .XFD image");
    return -2;
  }
  convertfname(file,fname);

  /* check free sectors */
  readVTOC(xfd);
  secsfree=VTOCsec[3]+VTOCsec[4]*256;

  /* input too big??? */
  if ((secsfree*125)<max) {
    fclose(xfd);
    error_dialog("No room on image");
    return -1;
  }

  /* find place in directory */
  entrynum=find_newentry(xfd);
  if (entrynum==-1) {
    error_dialog("No room on image");
    return -2;
  }

  startsec=scandir(xfd,fname);
  if (startsec>=0)  {
    error_dialog("Filename exists!");
    return -3;
  }

  /* find first free sector (=startsec) */
  startsec=find_free_sec(xfd);

  /* write directory entry */
  writedirentry(xfd,fname,startsec,max,entrynum);

  /* write file! */
  for(x=0;x<(max/125); x++) {
    cursec=find_free_sec(xfd);
    marksec(xfd,cursec,&secsfree);
    nextsec=find_free_sec(xfd);
    if (in) {
      fread(data,125,1,in);
      memcpy(secbuf,data,125);
    } else memcpy(secbuf,data+x*125,125);

    secbuf[125]=(entrynum<<2)+((highb(nextsec))&0x03);
    secbuf[126]=lowb(nextsec);
    secbuf[127]=125;
    writesec(xfd,cursec);
  }

  qwe=max%125;
  x=max/125;

  cursec=find_free_sec(xfd);
  marksec(xfd,cursec,&secsfree);

  if (in) {
    fread(data,125,1,in);
    memcpy(secbuf,data,qwe);
  } else memcpy(secbuf,data+x*125,qwe);
  secbuf[125]=(entrynum<<2);
  secbuf[126]=0;
  secbuf[127]=qwe;
  writesec(xfd,cursec);

  /* close file and exit */
  fclose(xfd);
  return 0;
}
/*=========================================================================*/
int import_palette(char *file, rgb_color * colortable)
{
	FILE *in;
	long len;
		
	in=fopen(file,"rb");
	if (!in) {
		error_dialog("Cannot open file");
		return -1;
	}
	
	if (flength(in)!=768) {
		error_dialog("Incorrect palette file (length<>768)");
		fclose(in);
	} else {
		len=fread(colortable,1,768,in);
		if (len!=768) 
		{
			error_dialog("Could not read. Reset to default.");
			fclose(in);
			handler_palette_reset();
			return 0;
		}
	}
	fclose(in);
	return 1;
}
/*=========================================================================*/
int read_font(char *file, unsigned char *font)
{
  FILE *in;

  in=fopen(file,"rb");
  if (!in) {
    error_dialog("Cannot open font");
    return -1;
  }
  fread(font,1024,1,in);
  fclose(in);
  return 0;
}
/*=========================================================================*/
int write_font(char *file, unsigned char *font)
{
  FILE *out;

  out=fopen(file,"wb");
  if (!out) {
    error_dialog("Cannot save font");
    return -1;
  }
  fwrite(font,1024,1,out);
  fclose(out);
  return 0;
}
/*=========================================================================*/
int write_data(char *file, unsigned char *font, int start, int end, int a)
{
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
  out=fopen(file,"wb");
  if (!out) {
    error_dialog("Cannot write font");
    return -1;
  }
  line=options.base;
  switch (options.write_tp) {
    case 1: { fprintf(out,"FONT%c",cr); break; }
    case 2: { fprintf(out,"%d FONT%c",line,cr); line+=options.step; break; }
	  case 3: { fprintf(out,"byte array FONT=[%c",cr); break; }
	  case 4: { fprintf(out,"unsigned char FONT[]={%c",cr); break; }
  }
	for(i=start;i<=end;i++) {
		switch (options.write_tp) {
			case 0: { fprintf(out,"%d DATA ",line); break; }
			case 1: { fprintf(out,"%c.BY ",tb); break; }
			case 2: { fprintf(out,"%d%c.BYTE ",line,tb); break; }
			case 3: { fprintf(out,"%c",tb); break; }
			case 4: { fprintf(out,"%c",tb); break; }
		}
		line+=options.step;
		for(j=0;j<8;j++) {
			c=*font;
			font++;
			
			if (options.write_tp==4)
				fprintf(out,"0x%02x",c);
			else
				fprintf(out,"%d",c);
			
			if (j!=7) {
				if (options.write_tp!=3) fprintf(out,",");
				else fprintf(out," ");
			} else {
				if (j==7) {
					if (options.write_tp==4)
						if (i!=end)
							fprintf(out,",");
				}
				fprintf(out,"%c",cr);
			}
		}
		
		if ((options.write_tp==1)&&(!((i+1)%32)))
			fprintf(out,"%c",cr);
	}
  if (options.write_tp==3)
    fprintf(out,"]");
	if (options.write_tp==4)
		fprintf(out,"};");

  fclose(out);
  return 0;
}
/*=========================================================================*/
int write_xfd_data(char *image, char *file, unsigned char *data, int start, int end)
{
  char *fname="~nvision.~mp";
  unsigned char buf[128];
  FILE *in;
  long len;

  write_data(fname,data,start,end,1);
  in=fopen(fname,"rb");
  if (!in) {
    error_dialog("Cannot write font");
    return -1;
  }
  len=flength(in);
  write_xfd_font(image,file,buf,len,in);
  fclose(in);
  remove(fname);
  return 0;
}
/*=========================================================================*/
view *read_file_map(char *file, unsigned char *font, view *map, int raw)
{
	FILE *in;
	unsigned char head[16];
	int tp,i;
	int tile_inited=0;

	in=fopen(file,"rb");
	if (!in) {
		error_dialog("Cannot open map");
		return NULL;
	}
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

int write_map(char *image, char *file, unsigned char *font, view *map, int raw)
{
	int res;
	if (image)
		res=write_xfd_map(options.disk_image,file,font,map,raw);
	else
		res=write_file_map(file,font,map,raw);
	
	return res;
}


/*=========================================================================*/
int write_file_map(char *file, unsigned char *font, view *map, int raw)
{
	FILE *out;
	unsigned char head[16];
	
	out=fopen(file,"wb");
	if (!out) {
		error_dialog("Cannot save map");
		return -1;
	}
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
int write_xfd_map(char *image, char *file, unsigned char *font, view *map, int raw)
{
  char *fname="~nvision.~mp";
  unsigned char buf[128];
  FILE *in;
  long len;

  if TILE_MODE {
    error_dialog("No tilemap.XFD save");
    return -1;    
  }

  write_file_map(fname,font,map,raw);
  in=fopen(fname,"rb");
  if (!in) {
    error_dialog("Cannot write map");
    return -1;
  }
  len=flength(in);
  write_xfd_font(image,file,buf,len,in);
  fclose(in);
  remove(fname);
  return 0;
}

view *read_map(char *image, char *file, unsigned char *font, view *map, int raw)
{
	if (image)
		return read_xfd_map(image,file,font,map,raw);
	else
		return read_file_map(file,font,map,raw);
}


/*=========================================================================*/
view *read_xfd_map(char *image, char *file, unsigned char *font, view *map, int raw)
{
  unsigned char head[128];
  int i;
	if (!raw) {
		i=read_xfd_font(image,file,head,10);
		if (i<0) return NULL;
		mode=head[0];
		map->cx=map->cy=map->scx=map->scy=0;
		map->w=head[1]+head[2]*256;
		map->h=head[3]+head[4]*256;
		mask->cx=mask->cy=mask->scx=mask->scy=0;
		mask->w=map->w;
		mask->h=map->h;
		clut[0]=head[5];
		clut[1]=head[6];
		clut[2]=head[7];
		clut[3]=head[8];
		clut[4]=head[9];
	}
	if (!raw || FILE_RAWMAP){
		if (map->map) { free(map->map); }
		// dirty hack, read whole file at a time (JH)
		// there might be an mask after fonts with "2" id
		map->map=(unsigned char *)malloc(map->w*map->h*2+1034+1);
	}
	
	
	if (FILE_RAWMASK || FILE_RAWBITMASK) {
		if (mask->map) { free(mask->map); }
		mask->map=(unsigned char *)malloc(mask->w*mask->h);
		memset(mask->map,0,mask->w*mask->h);
	}
	
	if (!raw || raw==FILE_RAWMAP) {
		memset(map->map,0,map->w*map->h);
		i=read_xfd_font(image,file,map->map,map->w*map->h*2+1034+1);
		if (i<0) return NULL;
		
		if (raw==FILE_RAWMAP)
			memset(map->map+map->w*map->h, 0, map->w*map->h+1034+1);
	}
	if (raw==FILE_RAWMASK || raw==FILE_RAWBITMASK) {
		i=read_xfd_font(image,file,mask->map,mask->w*mask->h);
		if (i<0) return NULL;
	}
	
	if (raw==FILE_RAWBITMASK) {
		unsigned char * look=mask->map+mask->w*mask->h;
		unsigned char * poke=mask->map+mask->w*mask->h;
		int i;
		for (i=mask->w*mask->h-1; i>=0; i--)
		{
			look=mask->map+i/8;
			*(--poke)=!!(*look&1<<(7-(i&0x7)));
		}
	}
		
	
	if (!raw) {
		memmove(map->map,map->map+10,map->w*map->h);
		memcpy(font,map->map+10+map->w*map->h,1024);
		if (map->map[10+map->w*map->h+1024]==2)
			memcpy(mask->map,map->map+1035+map->w*map->h,map->w*map->h);
		
	}
	return map;
}
/*=========================================================================*/
