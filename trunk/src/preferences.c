#include "preferences.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "envision.h"


CONFIG_ENTRY CONFIG_ENTRIES[CONFIG_ENTRY_CNT];
static char error_message[64];


#if defined(_WIN64) || defined(__WIN64__) || defined(WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// APPDATA 
char * get_preferences_filepath()
{ 
	static char buf [256];
	char* result;
	result = getenv("APPDATA");
	if (!result) return NULL;
	sprintf (buf,"%s\\%s",result,"envisionPCreborn.dat");
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
	sprintf (buf,"%s/%s/%s",result,"Library/Preferences","envisionPCreborn.pref");
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
	sprintf (buf,"%s/%s",result,".envisionPCreborn.dat");
	return buf;
}
#endif

int skip_to_eol(FILE * fd)
{
	int tmp;
	while (EOF!=(tmp=fgetc(fd))) {
		if (tmp=='\n') return 0;
	};
	return EOF;
}

/* readpref
 * reads one preference entry from opened file and tries to store it in aprorpiate place in CONFIG_ENTRIES by overwriting it.
 * if there is no such entry, preference entry is skipped;
 * if there are wrong values, preference entry is skipped; 
 * 
 */

int readpref(FILE * fd)
{
	char localbuf[MAX_PREFS_LENGTH];
	int len, tmp, i;
	CONFIG_ENTRY * prefset;
	
	if( EOF==fscanf(fd,"%s\n",localbuf)) return EOF;
	
	if( EOF==fscanf(fd,"%d\n",&len)) return EOF;
	if (len>MAX_PREFS_LENGTH) return skip_to_eol(fd);
	
	prefset = find_config_entry(localbuf);
	if (!prefset) return skip_to_eol(fd);
		
	char * bptr=localbuf;
	for (i=0;i<len; i++)
	{
		if (feof(fd)) return EOF;
		if ('\n'==(tmp=fgetc(fd))) return 0;
		if (0>(tmp=hex2dec(tmp))) { return skip_to_eol(fd); }
		
		*bptr=tmp<<4;
		
		if (feof(fd)) return EOF;
		if ('\n'==(tmp=fgetc(fd))) return 0;
		if (0>(tmp=hex2dec(tmp))) { return skip_to_eol(fd); }
		
		*bptr |= tmp;
		
		bptr++;
		}

	if (feof(fd)) return EOF;
	if ('\n'==fgetc(fd))
	{
		// zeroify in case of shorter prefs in file
		memset(prefset->buffer, 0, prefset->len);

		memcpy(prefset->buffer, localbuf, len);
		return 1;
	};
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
		if (EOF==fputc(hex[((*bptr)>>4)&0xf],fd)) return 0;
		if (EOF==fputc(hex[(*bptr)&0xf],fd)) return 0;
		bptr++;
	}
	if (EOF==fputc('\n',fd)) return 0;
	return 1;
}

int setprefs()
{
	CONFIG_ENTRY * prefset=CONFIG_ENTRIES;
	FILE * fd;
	char * preffilename=get_preferences_filepath();
	int i;
	
	fd=fopen(preffilename,"wb");
	if (!fd) {
		set_error("Cannot open preferences file for writing.");
		return 0;
	}
	// iterate through CONFIG_ENTRIES
	
	for (i=0; i<sizeof(CONFIG_ENTRIES) / sizeof (CONFIG_ENTRY); i++) {
		if (!writepref(fd, prefset->pref_id, prefset->buffer, prefset->len))
		{
			set_error("Cannot write preferences file. Deleting.");
			fclose(fd);
			delete_prefs();
			return 0;
			
		}
		prefset++;
	}
	fclose(fd);
	return 1;
};


CONFIG_ENTRY * find_config_entry(char * name) {
	CONFIG_ENTRY * prefset = CONFIG_ENTRIES;
	int i;
	
	// iterate through CONFIG_ENTRIES
	for (i=0; i<sizeof(CONFIG_ENTRIES) / sizeof (CONFIG_ENTRY); i++) {
		
		if (strcmp(name, prefset->pref_id)==0)
		{
			return prefset;
		}
		prefset++;
	}
	return NULL;
}



int getprefs()
{
	FILE * fd;
	int res;
	char * preffilename=get_preferences_filepath();

	res=1;
	error_message[0]=0;
	
	fd=fopen(preffilename,"rb");
	if (!fd) {
		res = setprefs();
		if (!res) set_error("Cannot open preferences file.");
		return 0;
	}
x	
	while (EOF!=readpref(fd));

	fclose(fd);
	return 1;
}

int delete_prefs()
{
	char * preffilename=get_preferences_filepath();
	return ! unlink(preffilename);
}
void set_error(const char * error)
{
	strncpy(error_message,error,63);
}

char * get_error()
{
	if (error_message[0]==0) return 0;
	return error_message;
}
