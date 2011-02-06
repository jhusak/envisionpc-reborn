#include "preferences.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "envision.h"
CONFIG_ENTRY CONFIG_ENTRIES[CONFIG_ENTRY_CNT];


#if defined(_WIN64) || defined(__WIN64__) || defined(WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// APPDATA 
char * get_preferences_filepath()
{ 
	static char buf [256];
	char* result;
	result = getenv("APPDATA");
	if (!result) return NULL;
	sprintf (buf,"%s\\%s\n",result,"envisionPCreborn.dat");
	return buf;
}

#elif defined(__APPLE__)
// ~/Library/Preferences
char * get_preferences_filepath()
{ 
	static char buf [256];
	char* result;
	result = getenv("HOME");
	if (!result) return NULL;
	sprintf (buf,"%s/%s/%s\n",result,"Library/Preferences","envisionPCreborn.pref");
	return buf;
}

#else
// linux/unix/etc, home catalog begun with . (dot)
char * get_preferences_filepath()
{ 
	static char buf [256];
	char* result;
	result = getenv("HOME");
	if (!result) return NULL;
	sprintf (buf,"%s/%s\n",result,".envisionPCreborn.dat");
	return buf;
}
#endif

int hex2dec(char hex)
{
	if (hex=='\n') return -1;
	if (!((hex>='0' && hex<='9') ||(hex>='A' && hex<='F'))) return -2;
	if (hex<='9') return hex-'0';
	return hex-'A'+10;
}

int readpref(FILE * fd, char * pref_id, char * buffer)
{
	char localbuf[128];
	int len, tmp, i;

	if( 0>fscanf(fd,"%s\n",localbuf)) return 0;
	if (!pref_id)
		strcpy(pref_id,localbuf);
	else if (strcpy(pref_id,localbuf)==0) {
		if( 0>fscanf(fd,"%d\n",&len)) return 0;
	}

	char * bptr=buffer;
	for (i=0;i<len; i++)
	{
		if (EOF==(tmp=fgetc(fd))) return 0;
		if (0>(tmp=hex2dec(tmp))) return 0;
		*bptr=tmp<<4;
		if (EOF==(tmp=fgetc(fd))) return 0;
		if (0>(tmp=hex2dec(tmp))) return 0;
		*bptr |= tmp;
		bptr++;
		}
	if (EOF==(tmp=fgetc(fd))) return 0;
	if (tmp=='\n') return 1;
	return 0;
}

int writepref(FILE * fd,  char * pref_id,  char * buf, int len)
{
	static char hex[]="0123456789ABCDEF";
	if (0>fprintf(fd,"%s\n",pref_id)) return 0;
	if (0>fprintf(fd,"%d\n",len)) return 0;

	int i;
	char * bptr=buf;
	for (i=0;i<len; i++)
	{
		if (EOF==fputc(hex[*bptr>>4],fd)) return 0;
		if (EOF==fputc(hex[*bptr&0xf],fd)) return 0;
		bptr++;
		}
	if (EOF==fputc('\n',fd)) return 0;
	return 1;
}

int setpref()
{
	CONFIG_ENTRY * prefset=CONFIG_ENTRIES;
	FILE * fd;
	char * preffilename=get_preferences_filepath();
	int i;
	
	fd=fopen(preffilename,"wb");
	if (!fd) {
		error_dialog("Cannot open preferences file for writing.");
		return 0;
	}
	// iterate through CONFIG_ENTRIES
	
	for (i=0; i<sizeof(CONFIG_ENTRIES) / sizeof (CONFIG_ENTRY); i++)
	if (!writepref(fd, prefset->pref_id, prefset->buffer, prefset->len))
	{
		error_dialog("Cannot write preferences file. Deleting.");
		fclose(fd);
		deleteprefs();
		return 0;

	}
	fclose(fd);
	return 1;
};

int getpref()
{
	CONFIG_ENTRY * prefset=CONFIG_ENTRIES;
	FILE * fd;
	int res,i;
	char * preffilename=get_preferences_filepath();
	
	fd=fopen(preffilename,"rb");
	if (!fd) {
		res = setpref();
		if (!res) error_dialog("Cannot open preferences file.");
		return 0;
	}
	// iterate through CONFIG_ENTRIES
	
	for (i=0; i<sizeof(CONFIG_ENTRIES) / sizeof (CONFIG_ENTRY); i++)
	if (!readpref(fd, prefset->pref_id, prefset->buffer))
	{
		error_dialog("Cannot read preferences file. Deleting.");
		fclose(fd);
		deleteprefs();
		return 0;
	}
	fclose(fd);
	return 1;
}

int deleteprefs()
{
	char * preffilename=get_preferences_filepath();
	return ! unlink(preffilename);
}