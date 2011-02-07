
#include <stdio.h>
typedef struct CONFIG_ENTRY {
	char pref_id[16];
	int len;
	void * buffer;
} CONFIG_ENTRY;

#define CONFIG_ENTRY_CNT 3
extern CONFIG_ENTRY CONFIG_ENTRIES[CONFIG_ENTRY_CNT];


char * get_preferences_filepath();
int readpref(FILE * fd, char * pref_id, char * buffer);
int writepref(FILE * fd, char * pref_id, char * buf, int len);

int setprefs();
int getprefs();

int delete_prefs();

void set_error(const char * error);
char * get_error();
