#ifndef __bitmap_h__
#define __bitmap_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include "uType.h"

#define color u32
#define getColor(a, b, c, d) (a|b<<8|c<<16|d<<24)

#define getA(c) ((c>> 0)&0x000000ff)
#define getR(c) ((c>> 8)&0x000000ff)
#define getG(c) ((c>>16)&0x000000ff)
#define getB(c) ((c>>24)&0x000000ff)

typedef struct{
	u32 height;
	u32 width;
	u32 byteperpixel;
	u8 *map;
}bitmap;

bitmap *createBitmap(u32 width, u32 height, u32 byteperpixel);

void destroyBitmap(bitmap *bm);

color getPixel(bitmap *bm, u32 x, u32 y);

void setPixel(bitmap *bm, u32 x, u32 y, color c);

bitmap *createBitmapWithInit(u32 width, u32 height, u32 byteperpixel, color c);

#endif
