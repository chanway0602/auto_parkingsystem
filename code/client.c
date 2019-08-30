#include  "myhead.h"
#include  "client.h"

/*TCP初始化*/
int TCPinit(service *serinfo)
{
	serinfo->TCPsock = socket(AF_INET, SOCK_STREAM, 0);	//创建数据流socket
	if(serinfo->TCPsock == -1)
	{
		perror("create TCP socket failed!");
		return -1;
	}

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(6666);
	saddr.sin_addr.s_addr = inet_addr("192.168.14.23");  //server ip (ubuntu ip)

	int ret = connect(serinfo->TCPsock, (struct sockaddr*)&saddr, sizeof(struct sockaddr));	
	if(ret == -1)
	{
		perror("connect failed!");
		return -1;
	}
	else if(ret == 0)
	{
		printf("connect successfully!\n");
		return 0;
	}

	return 0;
}


/*初始化串口com2*/
int RFIDinit(service *serinfo)
{
	//打开串口  com2
	serinfo->RFIDfd = open("/dev/ttySAC1", O_RDWR | O_NOCTTY );  //O_NONBLOCK
	if (serinfo->RFIDfd == -1)
	{
		perror("open gec6818 ttySAC1 failed!");
		return -1;
	}

	/*初始化串口，设置窗口参数:9600速率*/
	//声明设置串口的结构体
	struct termios termios_new;
	//先清空该结构体
	bzero( &termios_new, sizeof(termios_new));
	//	cfmakeraw()设置终端属性，就是设置termios结构中的各个参数。
	cfmakeraw(&termios_new);
	//设置波特率
	//termios_new.c_cflag=(B9600);
	cfsetispeed(&termios_new, B9600);
	cfsetospeed(&termios_new, B9600);
	//CLOCAL和CREAD分别用于本地连接和接受使能，因此，首先要通过位掩码的方式激活这两个选项。    
	termios_new.c_cflag |= CLOCAL | CREAD;
	//通过掩码设置数据位为8位
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8; 
	//设置无奇偶校验
	termios_new.c_cflag &= ~PARENB;
	//一位停止位
	termios_new.c_cflag &= ~CSTOPB;
	tcflush(serinfo->RFIDfd, TCIFLUSH);
	// 可设置接收字符和等待时间，无特殊要求可以将其设置为0
	termios_new.c_cc[VTIME] = 10;
	termios_new.c_cc[VMIN] = 1;
	// 用于清空输入/输出缓冲区
	tcflush (serinfo->RFIDfd, TCIFLUSH);
	//完成配置后，可以使用以下函数激活串口设置
	if(tcsetattr(serinfo->RFIDfd, TCSANOW, &termios_new) )
	{
		printf("Setting the serial1 failed!\n");
		return -1;
	}
	
	return 0;
}

/*计算校验和*/
unsigned char CalBCC(unsigned char *buf, int n)
{
	int i;
	unsigned char bcc=0;
	for(i = 0; i < n; i++)
	{
		bcc ^= *(buf+i);
	}
	return (~bcc);
}

/*发送A指令*/
int PiccRequest(service *serinfo)
{
	unsigned char WBuf[128], RBuf[128];

	int  ret, i;
	fd_set rdfd;
	static struct timeval timeout;

	memset(WBuf, 0, 128);
	memset(RBuf,  1, 128);
	WBuf[0] = 0x07;	//帧长= 7 Byte
	WBuf[1] = 0x82;	//包号= 0 , 命令类型= 0x02
	WBuf[2] = 'A';	//命令= 'A'
	WBuf[3] = 0x01;	//信息长度= 1
	WBuf[4] = 0x52;	//请求模式:  ALL=0x52
	WBuf[5] = CalBCC(WBuf, WBuf[0]-2);		//校验和
	WBuf[6] = 0x03; 	//结束标志

    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
	FD_ZERO(&rdfd);
	FD_SET(serinfo->RFIDfd,&rdfd);

	write(serinfo->RFIDfd, WBuf, 7);
	ret = select(serinfo->RFIDfd + 1,&rdfd, NULL, NULL, &timeout);
 	//printf("PiccRequest......................\n");
	switch(ret)
	{
		case -1:
			perror("PiccRequest select error!");
			break;
		case  0:
			printf("PiccRequest timed out!\n");
			break;
		default:
			ret = read(serinfo->RFIDfd, RBuf, 8);
			if (ret < 0)
			{
				printf("PiccRequest ret = %d, %m\n", ret, errno);
				break;
			}
			if (RBuf[2] == 0x00)	 	//应答帧状态部分为0 则请求成功  返回卡的类型
			{
			        FD_CLR(serinfo->RFIDfd,&rdfd);
			        printf("PiccRequest success\n");
					return 0;
			}
			break;
	}

	FD_CLR(serinfo->RFIDfd, &rdfd);
	return -1;
}

/*发送B指令*/
/*防碰撞，获取范围内最大ID*/
int PiccAnticoll(service *serinfo)
{
	unsigned char WBuf[128], RBuf[128];
	int ret, i;
	fd_set rdfd;
	struct timeval timeout;
	int ID;

	memset(WBuf, 0, 128);
	memset(RBuf, -1,128);
	WBuf[0] = 0x08;	//帧长= 8 Byte
	WBuf[1] = 0x02;	//包号= 0 , 命令类型= 0x02
	WBuf[2] = 'B';	//命令= 'B'
	WBuf[3] = 0x02;	//信息长度= 2
	WBuf[4] = 0x93;	//防碰撞0x93 --一级防碰撞
	WBuf[5] = 0x00;	//位计数0
	WBuf[6] = CalBCC(WBuf, WBuf[0]-2);		//校验和
	WBuf[7] = 0x03; 	//结束标志

	timeout.tv_sec = 300;
	timeout.tv_usec = 0;
	FD_ZERO(&rdfd);
	FD_SET(serinfo->RFIDfd,&rdfd);
	write(serinfo->RFIDfd, WBuf, 8);

	ret = select(serinfo->RFIDfd + 1,&rdfd, NULL,NULL,&timeout);
	switch(ret)
	{
		case -1:
			perror("PiccAnticoll select error\n");
			break;
		case  0:
			perror("PiccAnticoll Timeout:");
			break;
		default:
			ret = read(serinfo->RFIDfd, RBuf, 10);
			if (ret < 0)
			{
				printf("PiccAnticoll ret = %d, %m\n", ret, errno);
				break;
			}

			if (RBuf[2] == 0x00) //应答帧状态部分为0 则获取ID 成功
			{
				ID = (RBuf[7]<<24) | (RBuf[6]<<16) | (RBuf[5]<<8) | RBuf[4];
				serinfo->ID = ID;
				printf("PiccAnticoll The card ID is %d\n",ID);
				return 0;
			}

	}

	FD_CLR(serinfo->RFIDfd, &rdfd);
	return -1;
}

/*摄像头初始化*/
int CAMinit(service *serinfo)
{
	//1.打开摄像头设备
	serinfo->CAMfd = open("/dev/video0", O_RDWR);
	if(serinfo->CAMfd < 0)
	{
		perror("open camera error!");
		return -1;
	}
	
	//2.获取摄像头驱动信息
	struct v4l2_capability vcap;
	int ret = ioctl(serinfo->CAMfd, VIDIOC_QUERYCAP,&vcap);
	if(ret < 0)
	{
		perror("capture error!");
		return -1;
	}
	//printf("drv:%s\n", vcap.driver);
	//printf("card:%s\n", vcap.card);

	//3.配置属性
	//设置采集的视频格式
	struct v4l2_format fmt;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	#if 1
	//设置宽高
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	//视频格式
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	ret = ioctl(serinfo->CAMfd, VIDIOC_S_FMT, &fmt);
	if(ret < 0)
	{
		perror("set fmt error!");
		return -1;
	}
	#endif
	//查询当前的视频格式
	ret = ioctl(serinfo->CAMfd, VIDIOC_G_FMT, &fmt);
	if(ret < 0)
	{
		perror("get fmt error!");
		return -1;
	}
	char chr[8]={0};
	memcpy(chr, &fmt.fmt.pix.pixelformat, 4);
	//printf("fmt:%s\n", chr);
	//printf("w:%d\n",fmt.fmt.pix.width);
	//printf("h:%d\n",fmt.fmt.pix.height);


	//4.申请缓冲区队列
	struct v4l2_requestbuffers reqbuf;
	reqbuf.count = 4;//缓冲区个数
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;//映射
	ret = ioctl(serinfo->CAMfd, VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0)
	{
		perror("request error!");
		return -1;
	}
	
	struct MFrame
	{
		unsigned char *start;
		int length;
	};

	memset(serinfo->mFrames, 0, sizeof(serinfo->mFrames));//清空

	//5.映射缓冲区到用户空间
	int i=0; 
	for(i=0; i<reqbuf.count; i++)
	{
		struct v4l2_buffer  vbuf;
		vbuf.index = i;
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(serinfo->CAMfd, VIDIOC_QUERYBUF, &vbuf);//从内核中取出一个缓存区
		if(ret < 0)
		{
			perror("querybuf error!");
			return -1;
		}
		//映射
		serinfo->mFrames[i].length = vbuf.length;
		serinfo->mFrames[i].start = mmap(NULL, vbuf.length, PROT_READ|PROT_WRITE, MAP_SHARED,serinfo->CAMfd, vbuf.m.offset);
		//把拿出来的缓冲区放回队列
		ret = ioctl(serinfo->CAMfd, VIDIOC_QBUF, &vbuf);
		if(ret < 0)
		{
			perror("qbuf error!");
			return -1;
		}
	}

	//6.开始采集数据
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(serinfo->CAMfd, VIDIOC_STREAMON, &type);
	if(ret < 0)
	{
		perror("capture error!");
		return -1;
	}

	return 0;

}

/*摄像头拍照*/
int TakePhoto(service *serinfo)
{

	//7.采集数据
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(serinfo->CAMfd, &rfds);
	select(serinfo->CAMfd+1, &rfds, NULL, NULL, NULL);
	//while(1)
	{
		struct v4l2_buffer readbuf;
		readbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		readbuf.memory = V4L2_MEMORY_MMAP;
		int ret = ioctl(serinfo->CAMfd, VIDIOC_DQBUF, &readbuf);
		if(ret < 0)
		{
			perror("dqbuf error");
			return -1;
		}

		char buf[30];
		memset(buf, 0, 30);
		time_t times;
		time(&times);
		struct tm *tv = gmtime(&times);
		memcpy(&(serinfo->ParkTime), tv, sizeof(struct tm));	//记录时间

		// 16-40-35_2018-08-20
		sprintf(buf, "%d-%d-%d_%d-%d-%d.jpg", (tv->tm_hour), (tv->tm_min), (tv->tm_sec), 
													(tv->tm_year+1900), (tv->tm_mon+1), (tv->tm_mday));  
		strcpy(serinfo->PicName, buf);	//记录图片名

		//保存一帧数据
		FILE *file = fopen(buf, "w+");
		fwrite(serinfo->mFrames[readbuf.index].start, readbuf.length, 1, file);
		fclose(file);

		//把readbuf放回队列
		ret = ioctl(serinfo->CAMfd, VIDIOC_QBUF, &readbuf);
		if(ret < 0)
		{
			perror("qbuf error");
			return -1;
		}

	}

	//8.停止采集
	//ret = ioctl(serinfo->CAMfd, VIDIOC_STREAMOFF, &type);

	return 0;
}

/*发送数据包*/
int SendPack(service *serinfo)
{
	char buf[256];
	packet pack;
	memset(buf, 0, 256);
	memset(&pack, 0, sizeof(packet));

	pack.ID = serinfo->ID;
	strcpy(pack.PicName, serinfo->PicName);
	memcpy(&(pack.ParkTime), &(serinfo->ParkTime), sizeof(struct tm));

	int ret = send(serinfo->TCPsock, (char *)&pack, sizeof(packet), 0);	//发送
	if(ret == -1)
	{
		perror("send failed!");
		return -1;
	}
	ret = recv(serinfo->TCPsock, buf, sizeof(buf), 0);								//接收应答
	if(ret == -1)
	{
		perror("recv failed!");
		return -1;
	}

	memcpy(&pack, (packet *)buf, sizeof(packet));								//处理数据包
	serinfo->is_parking = pack.is_parking;
	serinfo->cost = pack.cost;
	printf("[cost]:%d\n", serinfo->cost);
	serinfo->is_ok = true;		//接收到数据包

	if(pack.is_parking == 0)
	{
		printf("入库！\n");
	}
	if(pack.is_parking == 1)
	{
		printf("出库！\n");
	}

	return 0;
}

/*上传图片到服务器*/
int SendPic(service *serinfo)
{
	int fd = open(serinfo->PicName, O_RDONLY);
	if(fd == -1)
	{
		perror("open picture failed!");
		return -1;
	}

	//获取图片大小并发送过去
	struct stat picinfo;
	stat(serinfo->PicName, &picinfo);
	send(serinfo->TCPsock, (char *)&picinfo.st_size, sizeof(picinfo.st_size), 0);

	char buf[1024];
	memset(buf, 0, 1024);

	int Rnum;
	while(1)
	{
		Rnum = read(fd, buf, 1024);
		if(Rnum == 0)
		{
			printf("send picture finished!\n");
			break;
		}

		send(serinfo->TCPsock, buf, Rnum, 0);
		memset(buf, 0, 1024);
	}

	close(fd);
	return 0;
}


//初始化LCD
struct LcdDevice *init_lcd(const char *device)
{
	struct LcdDevice* lcd = malloc(sizeof(struct LcdDevice));
	if(lcd == NULL) return NULL;
	//1打开设备
	lcd->fd = open(device, O_RDWR);
	if(lcd->fd < 0)
	{
		perror("open lcd fail");
		free(lcd);
		return NULL;
	}

	//2.获取lcd设备信息
	struct fb_var_screeninfo info; //存储lcd信息结构体--在/usr/inlucde/linux/fb.h文件中定义
	int ret = ioctl(lcd->fd, FBIOGET_VSCREENINFO, &info);
	if(ret < 0)
	{
		perror("request fail");
	}

	lcd->width = info.xres;
	lcd->height = info.yres;
	lcd->pixByte = info.bits_per_pixel/8;//每一个像素占用的字节数
	
	//映射
	lcd->mp = mmap(NULL, lcd->width*lcd->height*lcd->pixByte, 
				    PROT_READ|PROT_WRITE,MAP_SHARED, lcd->fd, 0);
	if(lcd->mp == (void *)-1)
	{
		perror("mmap fail");		
	}
	//给lcd设置默认颜色
	lcd->color = 0x000ff00f;
	
	return lcd;
}


/*LCD显示线程*/
void *LCDdisplay(void *arg)
{
	service *serinfo = (service *)arg;

	//初始化LCD
	struct LcdDevice* lcd = init_lcd("/dev/fb0");	
	memset(lcd->mp, 0xff, 800*480*4);
	
	// 打开字体
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	fontSetSize(f, 72);
	// 创建bitmap
	bitmap *bm = createBitmap(800, 480, 4);		//调整画布大小  第一参数是列800 二参数是行480

	char buf[256];
	memset(buf, 0, 256);

	volatile int cost;
	while(1)
	{
		time_t times;
		time(&times);
		struct tm *tv = gmtime(&times);	//获取当前时间
		// 2018年08月20日 16:40:35
		sprintf(buf, "%d年%d月%d日     %d:%d:%d\n",  
		(tv->tm_year+1900), (tv->tm_mon+1), (tv->tm_mday),
		(tv->tm_hour), (tv->tm_min), (tv->tm_sec));  


		if(serinfo->is_ok == true)
		{
			char tmp[100];
			memset(tmp, 0, 100);
			if(serinfo->is_parking == 0)
			{
				sprintf(tmp, "%d：刷卡成功！\n欢迎来到无人停车场！\n", serinfo->ID );
				strcat(buf, tmp);
				memset(tmp, 0, 100);
				sprintf(tmp, "入库时间:\n%d年%d月%d日     %d:%d:%d\n", 
						(serinfo->ParkTime.tm_year+1900), (serinfo->ParkTime.tm_mon+1), (serinfo->ParkTime.tm_mday),
						(serinfo->ParkTime.tm_hour), (serinfo->ParkTime.tm_min), (serinfo->ParkTime.tm_sec));
				strcat(buf, tmp);
			}

			if(serinfo->is_parking == 1)
			{
				sprintf(tmp, "%d：刷卡成功！\n一路顺风！\n", serinfo->ID );
				strcat(buf, tmp);
				memset(tmp, 0, 100);
				sprintf(tmp, "出库时间:\n%d年%d月%d日     %d:%d:%d\n", 
						(serinfo->ParkTime.tm_year+1900), (serinfo->ParkTime.tm_mon+1), (serinfo->ParkTime.tm_mday),
						(serinfo->ParkTime.tm_hour), (serinfo->ParkTime.tm_min), (serinfo->ParkTime.tm_sec));
				strcat(buf, tmp);

				memset(tmp, 0, 100);
				sprintf(tmp, "收费：%d元\n", serinfo->cost);
				strcat(buf, tmp);
			}

			//serinfo->is_ok = false;
		}
		

		fontPrint(f, bm, 0, 0, buf, getColor(255, 0, 0,255), 0);
		memcpy(lcd->mp, bm->map, 800*480*4);

		memset(bm->map, 0xff, 800*480*4);
		memset(buf, 0, 256);
	}
	
	// 关闭字体
	fontUnload(f);
	// 关闭bitmap
	destroyBitmap(bm);
	
	pthread_exit(NULL);

}



int main(int argc, char **argv)
{

	service *serinfo = calloc(1, sizeof(service));

	//初始化TCP
	if( TCPinit(serinfo) == -1 )
	{
		printf("TCPinit failed!\n");
	}


	//初始化RFID
	if( RFIDinit(serinfo) == -1 )
	{
		printf("RFIDinit failed!\n");
	}

	//初始化摄像头
	if( CAMinit(serinfo) == -1 )
	{
		printf("CAMinit failed!\n");
	}

	//创建显示LCD界面的线程
	pthread_t tid;
	pthread_create(&tid, NULL, LCDdisplay, (void *)serinfo);

	printf("请刷卡！\n");
	while(1)
	{
		if( PiccRequest(serinfo) == 0 )	//发送请求指令
		{
				if( PiccAnticoll(serinfo) == 0) 	//防碰撞
				{
					printf("正在拍照...\n");
					if( TakePhoto(serinfo) == -1 )	//拍照
					{
						printf("Take Photo failed!\n");
					}
					printf("拍照完成！\n");

					if( SendPack(serinfo) == -1)	//发送数据包
					{
						printf("Send Packet failed!\n");
					}

					if( SendPic(serinfo) == -1)	//发送图片
					{
						printf("Send Picture failed!\n");
					}

					sleep(1);
				}

		}

		//free(serinfo);
	}


	//关闭摄像头
	//close(serinfo->CAMfd);

	return 0;
}

