/*=========================================================================
 * File: undo.c - map and tile utility classes                            *
 * Author: Jakub Husak                                                    *
 * Started: 03/19/2011                                                    *
 =========================================================================*/

void initUndo();
void clearUndo(int bnum);
void storeUndo(int bnum, unsigned char * mem, size_t size);
void getUndo(int bnum, unsigned char ** mem);