/*=========================================================================
 *           Envision: Font editor for Atari                              *
 ==========================================================================
 * File: util.c - map and tile utility classes                            *
 * Author: Mark Schmelzenbach                                             *
 * Started: 02/24/2006                                                    *
 =========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "envision.h"

/*==========================================================================*/
/* uses Fowler/Noll/Vo hash */
#define FNV_32_PRIME ((unsigned int)0x01000193) /* 32 bit magic FNV-1a prime */
#define FNV1_32_INIT ((unsigned int)0x811c9dc5) /* 32 bit initial basis */

/*==========================================================================*/
typedef struct hashNode {
  int id;
  unsigned char value[64];
  struct hashNode *nxt;
} hashNode;

/*==========================================================================*/
typedef struct hashTable {
  int next_id;
  hashNode **table;
} hashTable;

/*===========================================================================
 * killTable
 * destroy a hashtable structure
 * param table: the table to destroy
 *==========================================================================*/
void killTable(hashTable *table) {
  hashNode *kill, *nxt;
  int i;
  for(i=0;i<256;i++) {
    kill=table->table[i];
    while(kill) {
      nxt=kill->nxt;
      free(kill);
      kill=nxt;
    }
  }
  free(table->table);
  free(table);
}

/*===========================================================================
 * getHashTable
 * allocate a hashtable structure
 * return: the hashtable
 *==========================================================================*/
hashTable *getHashTable() {
  int i;
  hashTable *ret=(hashTable *)malloc(sizeof(hashTable));
  ret->table=(hashNode **)malloc(256*sizeof(hashNode *));
  for(i=0;i<256;i++) {
    ret->table[i]=NULL;
  }
  ret->next_id=0;
  return ret;
}

/*===========================================================================
 * hash_tile
 * retrieve an id for a tile, inserting into the hashtable if necessary
 * param hash: the hashtable to check
 * param tile: the tile to insert
 * return: the tile id
 *==========================================================================*/
int hash_tile(hashTable *hash, unsigned char *tile) {
  hashNode *walk, *lst;
  const unsigned char *be,*bp=tile;
  unsigned int hval = FNV1_32_INIT;
  int key;
  hashNode *newNode;

  be=bp+64;
  while (bp<be) { /* FNV-1a hash each byte in the buffer */
    hval^=(const unsigned int)*bp++; /* xor bottom with current octet */
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hval+=(hval<<1)+(hval<<4)+(hval<<7)+(hval<<8)+(hval<<24);
  }
  key=hval&255;
  lst=walk=hash->table[key];
  while(walk) {
    if (!memcmp(walk->value,tile,64)) {
      return walk->id;
    }
    lst=walk;
    walk=walk->nxt;
  }
  newNode=(hashNode *)malloc(sizeof(hashNode));
  memcpy(newNode->value,tile,64);
  newNode->id=hash->next_id++;
  newNode->nxt=NULL;

  if (hash->table[key]) {
    lst->nxt=newNode;
  } else {
    hash->table[key]=newNode;
  }
  return newNode->id;
}

/*===========================================================================
 * store_tile
 * update the tile map to reflect tiles in hashmap
 * param table: the source hashtable
 * param tile: the map to update
 *==========================================================================*/
void store_tiles(hashTable *table, view *tile) {
  hashNode *look;
  unsigned char *walk, *tileDef, *tiles[256];
  int i,w,h;

  for(i=0;i<256;i++) {
    tiles[i]=NULL;
  }
  for(i=0;i<256;i++) {
    look=table->table[i];
    while(look) {
      tiles[look->id]=look->value;
      look=look->nxt;
    }
  }
  for(i=0;i<256;i++) {
    int tx,ty;

    tileDef=tiles[i];
    if (!tileDef)
      break;
    ty=i/16;
    tx=i%16;
    walk=tile->map+tx*tsx+ty*tsy*tsx*16;
    for(h=0;h<tsy;h++) {
      for(w=0;w<tsx;w++) {
        *walk++=*tileDef++;
      }
      walk+=tile->w-tsx;
    }
  }
}

/*===========================================================================
 * tile_map
 * try to convert a 1x1 map to a nxm tiled map, if possible
 * param itsx: requested tile width
 * param itsy: requested tile height
 * param map: the source map
 * param tile: the tile map to update
 * returns: flag indicating success
 *==========================================================================*/
int tile_map(int itsx, int itsy, view *map, view *tiles) {
  int num;
  int tx,ty,w,h,i,j;
  unsigned char *look, *tile, *tiled, *walk;
  unsigned char key[64], *newmap;
  hashTable *table; 
  
  if ((itsx==1)&&(itsy==1))
    return untile_map(map,tiles);
  
  table=getHashTable();
  w=map->w/itsx;
  h=map->h/itsy;
  walk=tiled=(unsigned char *)malloc(w*h);

  for(ty=0;ty<h;ty++) {
    for(tx=0;tx<w;tx++) {
      /* build tile at this location */
      tile=key;
      memset(tile,0,64);
      look=map->map+tx*itsx+ty*itsy*map->w;
      for(i=0;i<itsy;i++) {
        for(j=0;j<itsx;j++) {
          if ((tx+j>=map->w)||(ty+i>=map->h))
            *tile++=0;
          else
            *tile++=*look++;
        }
        look+=map->w-itsx;
      }
      num=hash_tile(table,key);
      if (num>255) {
        error_dialog("Too many unique tiles");
        /* error, clean up! */
        killTable(table);
        free(tiled);
        return 0;
      }
      *walk++=num;
    }
  }
  tile_size(itsx,itsy);
  store_tiles(table,tiles);

  map->w=w;
  map->h=h;
  map->cx=map->cy=map->scx=map->scy=0;
  newmap=(unsigned char *)realloc(map->map,w*h);
  map->map=newmap;
  memcpy(newmap,tiled,w*h);

  killTable(table);
  free(tiled);
  return 1;
}

/*===========================================================================
 * untile_map
 * try to convert a tile map to a character map (1x1)
 * param map: the source map
 * param tile: the tile map
 * returns: flag indicating success
 *==========================================================================*/
int untile_map(view *map, view *tiles) {
  int num;
  int mx,my,tx,ty,w,h,i,j;
  unsigned char *tiled,*look,*tile,*newmap,*walk;

  w=map->w;
  h=map->h;
  look=tiled=(unsigned char *)malloc(w*h);
  memcpy(tiled,map->map,w*h);

  map->w*=tsx; map->h*=tsy;
  map->cx=map->cy=map->scx=map->scy=0;
  newmap=(unsigned char *)realloc(map->map,map->w*map->h);
  map->map=newmap;

  for(ty=0;ty<h;ty++) {
    for(tx=0;tx<w;tx++) {
      num=*look++;
      my=num/16;
      mx=num-my*16;
      tile=tiles->map+mx*tsx+my*tsy*tsx*16;
      walk=map->map+tx*tsx+ty*tsy*map->w;
      for(i=0;i<tsy;i++) {
        for(j=0;j<tsx;j++) {
	  *walk++=*tile++;
        }
        walk+=map->w-tsx;
	tile+=tsx*15;
      }
    }
  }  
  free(tiled);
  tile_size(1,1);
  return 1;
}
/*==========================================================================*/
