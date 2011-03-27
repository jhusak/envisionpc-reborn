/*=========================================================================
 * File: undo.c - map and tile utility classes                            *
 * Author: Jakub Husak                                                    *
 * Started: 03/19/2011                                                    *
 =========================================================================*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "undo.h"
#define UNDO_BUFNUM 3
static unsigned char * stored_undo[UNDO_BUFNUM];
static size_t stored_size[UNDO_BUFNUM];

void initUndo()
{
	return;
	int i;
	for (i=0; i<UNDO_BUFNUM; i++) {
		stored_undo[i]=NULL;
		stored_size[i]=0;
	}
}

void clearUndo(int bnum)
{
	return;
	if (bnum>=UNDO_BUFNUM) return;
	if (stored_undo[bnum]) free(stored_undo[bnum]);
	stored_undo[bnum]=NULL;
	stored_size[bnum]=0;
}


void storeUndo(int bnum, unsigned char * mem, size_t size)
{
	return;
	if (bnum>=UNDO_BUFNUM) return;
	if (stored_undo) free(stored_undo);
	stored_undo[bnum]=malloc(size);
	stored_size[bnum]=size;
	memcpy(stored_undo[bnum], mem, size);

}

void getUndo(int bnum, unsigned char **mem)
{
	return;
	unsigned char tmp;
	unsigned char * store;
	unsigned char * get;
	int i;
	
	if (bnum>=UNDO_BUFNUM) return;
	
	if (!stored_undo[bnum]) return;
	
	store=stored_undo[bnum];
	get=*mem;
	// swap between buffer values;
	// swap between buffers would be faster.
	for (i=0;i<stored_size[bnum]; i++)
	{
		tmp=*store;
		*store++=*get;
		*get++=tmp;
	}	
	
}
