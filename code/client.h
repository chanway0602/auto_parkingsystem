#ifndef   _CLIENT_H_
#define  _CLIENT_H_


#include <sys/mman.h>
#include <termios.h> 
#include <sys/select.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "myhead.h"
#include "sqlite3.h"
#include "bitmap.h"
#include "font.h"

#include <linux/fb.h>
#include <math.h>



//视频帧结构体
struct MFrame
{
	unsigned char *start;
	int length;
};


//lcd设备结构体
struct LcdDevice
{
	int fd;
	int width;
	int height;
	int pixByte;  //保存一个像素占用的字节数
	unsigned int *mp; //保存映射首地址
	unsigned int color;//默认颜色
};



typedef struct packet
{
	int ID;
	int is_parking;	//汽车是否在库的标志位
	int cost;				//计算停车费用
	struct tm ParkTime;
	char PicName[30];

}packet;



//信息管理结构体
typedef struct service
{
	int TCPsock;
	int LCDfd;
	int RFIDfd;
	int CAMfd;
	int cost;		//计算停车费用

	int ID;
	int is_parking;	//汽车是否在库的标志位
	unsigned long * mmap_star; //定义LCD内存映射指针

	struct MFrame mFrames[4];	//存储视频帧的结构体数组
	struct tm ParkTime;

	bool is_ok; 	//接收完数据包的标志
	char PicName[30];

}service;



int TCPinit(service *serinfo);
int RFIDinit(service *serinfo);
int PiccRequest(service *serinfo);
int PiccAnticoll(service *serinfo);
int CAMinit(service *serinfo);
int TakePhoto(service *serinfo);
int SendPack(service *serinfo);
int SendPic(service *serinfo);

unsigned char CalBCC(unsigned char *buf, int n);

struct LcdDevice *init_lcd(const char *device);




#endif




