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
view *read_map(char *file, unsigned char *font, view *map, int raw)
{
  FILE *in;
  unsigned char head[16];
  int tp,i;

	in=fopen(file,"rb");
	if (!in) {
		error_dialog("Cannot open map");
		return NULL;
	}
	if (!raw) {
		fread(head,10,1,in);
		mode=head[0];
		map->cx=map->cy=map->scx=map->scy=0;
		map->cw=map->ch=0;
		map->w=head[1]+head[2]*256;
		map->h=head[3]+head[4]*256;
		clut[0]=head[5];
		clut[1]=head[6];
		clut[2]=head[7];
		clut[3]=head[8];
		clut[4]=head[9];
		
	}
	if (map->map) {
    free(map->map);
  }
  map->map=(unsigned char *)malloc(map->w*map->h);
  fread(map->map,map->w*map->h,1,in);
	
	if (!raw) {
		fread(font,1024,1,in);
		
		tsx=tsy=1;
		tile->w=16; tile->h=16;
		tile->dc=1;
		tile->ch=tile->cw=0;
		tile->cx=tile->cy=tile->scx=tile->scy=0;
		if (tile->map)
			free(tile->map);
		tile->map=NULL;
		
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
					tile->w=16*tsx; tile->h=16*tsy;
					tile->ch=map->ch;
					tile->cw=map->cw;
					tile->map=(unsigned char *)malloc(256*tsx*tsy);
					memset(tile->map,256*tsx*tsy,0);
					for(i=0;i<num;i++) {
						int tx,ty,w,h;
						ty=i/16;
						tx=i-ty*16;
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
			}
		}
		if (!tile->map) {
			tile->map=(unsigned char *)malloc(tile->w*tile->h);
			for(i=0;i<256;i++)
				tile->map[i]=i;
		}
	}
  fclose(in);
  return map;
}
/*=========================================================================*/
int write_map(char *file, unsigned char *font, view *map, int raw)
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
  fwrite(map->map,map->w*map->h,1,out);
  if (!raw) {
    fwrite(font,1024,1,out);
    if ((!tileMode)&&((tsx>1)||(tsy>1))) {
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
        tx=i-ty*16;
        look=tile->map+tx*tsx+ty*tsy*tsx*16;
        for(h=0;h<tsy;h++) {
          for(w=0;w<tsx;w++) {
            fputc(*look,out);
            look++;
          }
          look+=tile->w-tsx;
        }
      }
    } else {
      fputc(0,out); /* signal end of blocks */
    }
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

  if ((!tileMode)&&((tsx>1)||(tsy>1))) {
    error_dialog("No tilemap.XFD save");
    return -1;    
  }

  write_map(fname,font,map,raw);
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
		clut[0]=head[5];
		clut[1]=head[6];
		clut[2]=head[7];
		clut[3]=head[8];
		clut[4]=head[9];
	}
  if (map->map) {
    free(map->map);
  }
  map->map=(unsigned char *)malloc(map->w*map->h+1034);
  i=read_xfd_font(image,file,map->map,map->w*map->h+1034);
  if (i<0) return NULL;
	if (!raw) {
		memmove(map->map,map->map+10,map->w*map->h);
		memcpy(font,map->map+10+map->w*map->h,1024);
	}
  return map;
}
/*=========================================================================*/
