#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "jpeglib.h"
#include "jconfig.h"
int main(int argc,char **argv)
{
	int i,j;
	int lcdfd;
	int *lcdmem;
	int x=200;
	int y=100;
	//初始化lcd
	lcdfd=open("/dev/fb0",O_RDWR);
	if(lcdfd==-1)
	{
		printf("open lcd failed!\n");
		return -1;
	}
	//映射lcd
	lcdmem=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	if(lcdmem==NULL)
	{
		printf("map lcd failed!\n");
		return -1;
	}
	//定义jpg解压缩结构体变量,错误处理结构体变量
	struct jpeg_decompress_struct  jpgcom;
	struct jpeg_error_mgr jpgerr;
	//初始化刚才的两个变量
	jpgcom.err=jpeg_std_error(&jpgerr);
	jpeg_create_decompress(&jpgcom);
	 
	//打开你要显示的jpg
	FILE *jpgp=fopen(argv[1],"r+");
	if(jpgp==NULL)
	{
		perror("打开jpg失败!\n");
		return -1;
	}
	//获取解压缩源
	jpeg_stdio_src(&jpgcom,jpgp);
	//获取jpg的头信息
	jpeg_read_header(&jpgcom,TRUE);
	//开始解压缩
	jpeg_start_decompress(&jpgcom);
	printf("图片的宽:%d  高%d\n",jpgcom.image_width,jpgcom.image_height);
	//定义一个char类型指针用于存放读取到的一行RGB的值
	char *buf=calloc(1,jpgcom.image_width*3);
	
	//循环读取图片RGB的数值
	for(i=0; i<jpgcom.image_height; i++)
	{
		jpeg_read_scanlines(&jpgcom,(JSAMPARRAY)(&buf),1);
		//将buf中读取到RGB值填充到lcd上
		//一整行RGB    3字节--->4字节
		for(j=0; j<jpgcom.image_width; j++)//例如  图片50*40
			//第一行:lcdmem+0  ----->lcdmem+jpgcom.image_width
		    //第二行:lcdmem+800  ---->lcdmem+800+jpgcom.image_width
			 *(lcdmem+j+800*(y+i)+x)=buf[3*j+2]|buf[3*j+1]<<8|buf[3*j]<<16;
	}
	//释放资源
	jpeg_finish_decompress(&jpgcom);
	munmap(lcdmem,800*480*4);
	close(lcdfd);
	return 0;
}