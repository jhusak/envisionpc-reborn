
#include <stdio.h>
typedef struct CONFIG_ENTRY {
	char pref_id[16];
	int len;
	void (* init_handler)();
	void * buffer;
} CONFIG_ENTRY;

// maximum length of one prefs entry in bytes decoded
#define MAX_PREFS_LENGTH 10000

#define CONFIG_ENTRY_CNT 3
extern CONFIG_ENTRY CONFIG_ENTRIES[CONFIG_ENTRY_CNT];

#if defined(_WIN64) || defined(__WIN64__) || defined(WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define TILDE_ALLOWED 0
#elif defined(__APPLE__)
#define TILDE_ALLOWED 1
#else
// linux/unix/etc
#define TILDE_ALLOWED 1
#endif


char * get_preferences_filepath();
int readpref(FILE * fd);
int writepref(FILE * fd, char * pref_id, char * buf, int len);

CONFIG_ENTRY * find_config_entry(char * name);

int setprefs();
int getprefs();

int delete_prefs();

void set_error(const char * error);
char * get_error();
