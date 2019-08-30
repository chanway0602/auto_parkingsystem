#ifndef   _SERVER_H_
#define  _SERVER_H_

#include <sys/mman.h>
#include <termios.h> 
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "myhead.h"
#include "sqlite3.h"


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
	int newsock;

	sqlite3 *MyDb;

	struct packet pack;

}service;



int TCPinit(service *serinfo);
int DataBaseInit(service *serinfo);
int RecvPic(service *serinfo);
int RecvPack(service *serinfo);
int DelData(packet *pack, service *serinfo);
int FindTime(void* arg, int col, char** rowinfo, char** field);
int bits(int num);
int ColCost(packet *pack, service *serinfo);
int CheakParkingTimes(void* arg, int col, char** rowinfo, char** field);
int CheakCar(packet *pack, service *serinfo);
int AddData(packet *pack, service *serinfo);


#endif




