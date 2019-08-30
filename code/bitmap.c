#include "bitmap.h"


bitmap *createBitmap(u32 width, u32 height, u32 byteperpixel){
	bitmap *bm = (bitmap *)malloc(sizeof(bitmap));
	bzero(bm, sizeof(bitmap));
	bm->height = height;
	bm->width = width;
	bm->byteperpixel = byteperpixel;
	bm->map = (u8 *)malloc(width*height*byteperpixel);
	//bzero(bm->map, width*height*byteperpixel);
	memset(bm->map, 0xff, width*height*byteperpixel);
	return bm;
}

void destroyBitmap(bitmap *bm){
	bzero(bm->map, bm->height * bm->width * bm->byteperpixel);
	free(bm->map);
	bzero(bm, sizeof(bitmap));
	free(bm);
}

color getPixel(bitmap *bm, u32 x, u32 y){
	if(bm->byteperpixel == 3){
		u8 r = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 0);
		u8 g = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 1);
		u8 b = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 2);
		return getColor(0, r, g, b);
	}else if(bm->byteperpixel == 4){
		u8 r = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 0);
		u8 g = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 1);
		u8 b = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 2);
		u8 a = *(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 3);
		return getColor(a, r, g, b);
	}
	return 0;
}

void setPixel(bitmap *bm, u32 x, u32 y, color c){
	if(bm->byteperpixel == 3){
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 0) = getR(c);
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 1) = getG(c);
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 2) = getB(c);
	}else if(bm->byteperpixel == 4){
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 0) = getR(c);
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 1) = getG(c);
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 2) = getB(c);
		*(bm->map + y*bm->width*bm->byteperpixel + x*bm->byteperpixel + 3) = getA(c);
	}
}

bitmap *createBitmapWithInit(u32 width, u32 height, u32 byteperpixel, color c){
	bitmap *bm = createBitmap(width, height, byteperpixel);
	u32 x, y;
	for(y=0; y<height; y++){
		for(x=0; x<width; x++){
			setPixel(bm, x, y, c);
		}
	}
	return bm;
}

