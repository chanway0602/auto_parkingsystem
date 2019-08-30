#include "myhead.h"
#include "server.h"

/*TCP初始化*/
int TCPinit(service *serinfo)
{

	serinfo->TCPsock = socket(AF_INET, SOCK_STREAM, 0);	//创建数据流socket
	if(serinfo->TCPsock == -1)
	{
		perror("create TCP socket failed!");
		return -1;
	}

	int sinsize = 1;
	setsockopt(serinfo->TCPsock, SOL_SOCKET, SO_REUSEADDR, &sinsize, sizeof(int));	//设置socket属性,重复使用端口

	struct sockaddr_in myaddr;
	memset(&myaddr, 0, sizeof(struct sockaddr_in));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(6666);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);  

	if(bind(serinfo->TCPsock, (struct sockaddr *)&myaddr, sizeof(struct sockaddr)) == -1)		//绑定ip和端口
	{
		perror("TDP bind failed!");
		return -1;
	}

	if(listen(serinfo->TCPsock, 5) == -1)	//打开监听
	{
		perror("TCP listen failed!");
		return -1;
	}

	struct sockaddr_in caddr;
	memset(&caddr, 0, sizeof(struct sockaddr_in));
	int size = sizeof(struct sockaddr);
	serinfo->newsock = accept(serinfo->TCPsock, (struct sockaddr *)&caddr, &size);		//等待目标用户确认连接
	if(serinfo->newsock == -1)
	{
		perror("accept failed!");
		return -1;
	}

	return 0;

}

/*数据库初始化*/
int DataBaseInit(service *serinfo)
{
	if(sqlite3_open("ParkSystem.db", &(serinfo->MyDb)) != SQLITE_OK )
	{
		printf("open date base failed!\n");
		return -1;
	}

	sqlite3_exec(serinfo->MyDb, "create table ParkInfo(ID int,PicName char [30],InTime char [30],OutTime char [30]);", NULL, NULL, NULL);

	return 0;
}


/*TCP接收图片*/
int RecvPic(service *serinfo)
{
	int fd = open(serinfo->pack.PicName, O_WRONLY|O_CREAT, 0777);
	if(fd == -1)
	{
		perror("open file failed!");
		return -1;
	}

	off_t picsize;
	recv(serinfo->newsock, (char *)&picsize, sizeof(off_t), 0);		//接收图片大小信息

	char buf[1024];
	memset(buf, 0, 1024);

	int ret;
	while(1)
	{
		ret = recv(serinfo->newsock, buf, sizeof(buf), 0);
		if(ret == -1)
		{
			perror("TCP recv failed!\n");
			return -1;
		}
		else if(ret == 0)
		{
			printf("communicate interrupted!\n");
			return -1;
		}

		char *tmp = buf;
		int Wnum;
		while(ret > 0)
		{
			Wnum = write(fd, tmp, ret);
			tmp += Wnum;
			ret   -= Wnum;
		}

		//获取图片大小
		struct stat picinfo;
		stat(serinfo->pack.PicName, &picinfo);
		if(picinfo.st_size == picsize)
		{
			printf("接收图片完成\n");
			break;
		}

	}

	close(fd);
	return 0;
}


/*TCP接收数据包*/
int RecvPack(service *serinfo)
{
	char buf[256];
	packet pack;
	memset(buf, 0, 256);
	memset(&pack, 0, sizeof(packet));

	int ret = recv(serinfo->newsock, buf, sizeof(buf), 0);						//接收数据包
	if(ret == -1)
	{
		perror("recv failed!");
		return -1;
	}
	if(ret == 0)
	{
		exit(0);
	}

	memcpy(&pack, (packet *)buf, sizeof(packet));							//处理数据包
	memcpy(&(serinfo->pack), (packet *)buf, sizeof(packet));				//将数据包信息复制到serinfo->pack
	pack.is_parking = CheakCar(&pack, serinfo);								//检查该车辆是否在数据库
	if(pack.is_parking == 0)	//判断为入库
	{
		AddData(&pack, serinfo);	 														//把车辆信息添加到数据库
	}
	if(pack.is_parking == 1)	//判断为出库
	{
		//计算金额
		pack.cost = ColCost(&pack, serinfo);	//有返回值
		//删除数据
		DelData(&pack, serinfo);
	}
	

	ret = send(serinfo->newsock, (char *)&pack, sizeof(packet), 0);	 //应答客户端，告知客户端该车辆是否在库
	if(ret == -1)
	{
		perror("send failed!");
		return -1;
	}

	return 0;
}


/*删除数据库中某一ID的数据*/
int DelData(packet *pack, service *serinfo)
{
	char buf[100];
	memset(buf, 0, 100);

	sprintf(buf,   "delete from ParkInfo where ID=%d;", pack->ID);
	sqlite3_exec(serinfo->MyDb, buf, NULL, NULL, NULL);

	return 0;
}


/*select 查询时间的回调函数*/
int FindTime(void* arg, int col, char** rowinfo, char** field)
{
	//printf("rowinfo[2]%s\n", rowinfo[2]);
	memcpy((char *)arg, rowinfo[2], 30);
		
	return 0;
}


/*判断一个数的位数*/
int bits(int num)
{
	int sum = 0;
	while(num)
	{
        sum++;
        num/=10;
    }
    return sum;
}


/*计算金额*/
int ColCost(packet *pack, service *serinfo)
{
	char buf[100], time[30];
	memset(buf, 0, 30);
	memset(buf, 0, 100);

	sprintf(buf, "select * from ParkInfo where ID=%d;", pack->ID);
	sqlite3_exec(serinfo->MyDb, buf, FindTime, (void *)time, NULL);
	//printf("[time]:%s\n", time);
	int hour, min, sec, num;	//入库的时间
	char tmp[20];
	char *p = time;
	memset(tmp, 0 , 20);		//5:12:51_2015.1.1

	hour = atoi(p);
	num = bits(hour);
	p = p+num+1;
	min = atoi(p);
	num = bits(min);
	p = p+num+1;
	sec = atoi(p);
	//printf("[hour]:%d, [min]:%d, [sec]:%d\n", hour, min, sec);
	//printf("[h]:%d, [m]:%d, [s]:%d\n", pack->ParkTime.tm_hour, pack->ParkTime.tm_min, pack->ParkTime.tm_sec);

	int cost;
	if(pack->ParkTime.tm_sec >= sec)
	{
		cost = (pack->ParkTime.tm_min - min)*60 + (pack->ParkTime.tm_sec - sec);
	}
	else
	{
		cost = (pack->ParkTime.tm_min - min)*60 + pack->ParkTime.tm_sec - sec;
	}
	//printf("[cost]:%d\n", cost);

	return cost;
}



/*select查询数据库的回调函数*/
//接收来自第四个参数传递过来的数据;表示表格中的列数;每一行的信息;字段名;
int CheakParkingTimes(void* arg, int col, char** rowinfo, char** field)
{
	int *is_parking = (int *)arg;

	if(*is_parking == 1)
	{
		*is_parking = 0;
	}
	else
	{
		*is_parking = 1;
	}

	return 0;
}	


/*查询该车辆在库的停车次数，是入库还是出库*/
int CheakCar(packet *pack, service *serinfo)
{
	char buf[100];
	memset(buf, 0, 100);

	volatile int is_parking = 0;	//汽车是否在库的标志位

	sprintf(buf,   "select ID from ParkInfo where ID=%d;", pack->ID);
	sqlite3_exec(serinfo->MyDb, buf, CheakParkingTimes, (void *)&is_parking, NULL);

	return is_parking;
}


/*把停车信息加入到数据库*/
int AddData(packet *pack, service *serinfo)
{
	char buf[100], InTime[30], OutTime[30];
	memset(InTime, 0, 30);
	memset(OutTime, 0, 30);
	memset(buf, 0, 100);

	//20:45:12_2018.8.21
	sprintf(InTime, "%d:%d:%d_%d.%d.%d", pack->ParkTime.tm_hour, pack->ParkTime.tm_min, pack->ParkTime.tm_sec,
															 pack->ParkTime.tm_year+1900, pack->ParkTime.tm_mon+1, pack->ParkTime.tm_mday);	
	sprintf(buf,   "insert into ParkInfo values(\"%d\",\"%s\",\"%s\",\"%s\");", pack->ID, pack->PicName, InTime, OutTime);
	sqlite3_exec(serinfo->MyDb, buf, NULL, NULL, NULL);

	return 0;
}



int main(int argc, char **argv)
{
	service *serinfo = calloc(1, sizeof(service));

	//初始化数据库
	if(DataBaseInit(serinfo) == -1)
	{
		printf("Init DataBase failed!\n");
	}

	//初始化TCP
	TCPinit(serinfo);

	while(1)
	{
		if( RecvPack(serinfo) == -1)
		{
			printf("RecvPack failed!\n");
		}
		if( RecvPic(serinfo) == -1)
		{
			printf("RecvPic failed!\n");
		}

	}


	//关闭数据库
	sqlite3_close(serinfo->MyDb);
	free(serinfo);

	return 0;
}