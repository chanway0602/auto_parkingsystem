#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  //open的头文件
#include <unistd.h>
#include <sys/mman.h>//mmap的头文件
/*
	该文件中存放的全部都是关于bmp格式图片显示的函数
*/
//自己封装一个专门用于显示800*480大小bmp的函数
int show_bmp(char *bmppath)
{
	int lcdfd;
	int bmpfd;
	int i;
	int x,y;
	//定义一个数组用于存放读取到的RGB数值
	char bmpbuf[800*480*3];//char 占1字节
	int lcdbuf[800*480];//int 占4字节
	int tempbuf[800*480];
	//打开液晶屏的驱动   可读写的方式
	lcdfd=open("/dev/fb0",O_RDWR);
	//判断返回值  代码严谨
	if(lcdfd==-1)
	{
		printf("open lcd failed!\n");
		return -1;
	}
	//打开你要显示的bmp图片(显示全屏800*480大小的)
	bmpfd=open(bmppath,O_RDWR);
	if(bmpfd==-1)
	{
		printf("open 1.bmp failed!\n");
		return -1;
	}
	//跳过前面没有用的54字节，从55字节开始读取
	lseek(bmpfd,54,SEEK_SET);
	//读取bmp图片的RGB
	read(bmpfd,bmpbuf,800*480*3);
	//将读取到颜色值转换成4字节
	for(i=0; i<800*480; i++)
		lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;
	//将翻转的图片纠正(x,y) <---->(x,479-y)
	for(x=0; x<800; x++)
		for(y=0; y<480; y++)
			tempbuf[(479-y)*800+x]=lcdbuf[y*800+x];
	//将转换后的颜色值填充到lcd
	write(lcdfd,tempbuf,800*480*4);
	//关闭
	close(bmpfd);
	close(lcdfd);
	return 0;
}

/*
	该文件中存放的全部都是关于bmp格式图片显示的函数
	int a=12;
	int *p=&a;
	*p=15;
*/
//自己封装一个任意位置显示任意大小的bmp
int show_anybmp(int x,int y,int w,int h,char *bmppath)
{
	int lcdfd;
	int bmpfd;
	int *lcdmem;
	int i,j;
	int excess;
	//定义一个数组用于存放读取到的RGB数值
	char bmpbuf[w*h*3];//char 占1字节
	int lcdbuf[w*h];//int 占4字节
	int tempbuf[w*h];
	//打开液晶屏的驱动   可读写的方式
	lcdfd=open("/dev/fb0",O_RDWR);
	//判断返回值  代码严谨
	if(lcdfd==-1)
	{
		printf("open lcd failed!\n");
		return -1;
	}
	//映射lcd 获取lcd首地址
	lcdmem=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	//打开你要显示的bmp图片(显示w*h大小的)
	bmpfd=open(bmppath,O_RDWR);
	if(bmpfd==-1)
	{
		printf("open %s failed!\n",bmppath);
		return -1;
	}
	//跳过前面没有用的54字节，从55字节开始读取
	lseek(bmpfd,54,SEEK_SET);
	excess=4-(w*3)%4;
	//读取bmp图片的RGB   我们研究发现bmp要求宽4字节对齐  
	if((w*3)%4==0)
		read(bmpfd,bmpbuf,w*h*3);
	else //一行行的读取，剔除每一行添加进来的垃圾数据
	{
		for(i=0; i<h; i++)//bmpbuf[0]--[w*3-1]   bmpbuf[w*3]--[2*w*3-1]  
		{
			read(bmpfd,&bmpbuf[w*3*i],w*3);
			lseek(bmpfd,excess,SEEK_CUR);//换到下一行
		}
	}
	//将读取到颜色值转换成4字节
	for(i=0; i<w*h; i++)
		lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;
	//将转换好的ARGB赋值给到刚才映射到的首地址
	for(i=0; i<w; i++)
		for(j=0; j<h; j++)
			*(lcdmem+(y+j)*800+x+i)=lcdbuf[(h-1-j)*w+i];
			//*(lcdmem+(y+j)*800+x+i)=lcdbuf[j*w+i];  图片是颠倒的
	//关闭
	close(bmpfd);
	close(lcdfd);
	munmap(lcdmem,800*480*4);
	return 0;
}

