#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char const *argv[])
{
	//1.打开摄像头设备
	int vfd = open("/dev/video7", O_RDWR);

	//2.获得相关信息
	struct v4l2_capability vcap;
	int ret = ioctl(vfd, VIDIOC_QUERYCAP,&vcap);
	printf("card:%s\n", vcap.card);

	//3.//设置采集的视频格式
	struct v4l2_format fmt;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 

	//设置宽高
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;

	//视频格式
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	ioctl(vfd, VIDIOC_S_FMT, &fmt);

	//查询当前的视频格式
	ret = ioctl(vfd, VIDIOC_G_FMT, &fmt);
	if(ret < 0)
	{
		perror("get fmt error");
		exit(1);
	}

	char chr[8]={0};
	memcpy(chr, &fmt.fmt.pix.pixelformat, 4);
	printf("fmt:%s\n", chr);
	printf("w:%d\n",fmt.fmt.pix.width);
	printf("h:%d\n",fmt.fmt.pix.height);

	//4.申请缓冲区队列
	struct v4l2_requestbuffers reqbuf;
	reqbuf.count = 4;//缓冲区个数
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;//映射
	ret = ioctl(vfd, VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0)
	{
		perror("request error");
		exit(1);
	}

	//5.映射缓冲区到用户空间
	struct MFrame
	{
		unsigned char *start;
		int length;
	};

	struct MFrame mFrames[reqbuf.count];
	memset(mFrames, 0, sizeof(mFrames));//清空

	int i=0; 
	//for(i=0; i<reqbuf.count; i++)
	//{
		struct v4l2_buffer  vbuf;
		vbuf.index = i;
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(vfd, VIDIOC_QUERYBUF, &vbuf);//从内核中取出一个缓存区
		if(ret < 0)
		{
			perror("querybuf error");
			exit(1);
		}

		//映射
		mFrames[i].length = vbuf.length;
		mFrames[i].start = mmap(NULL, vbuf.length, PROT_READ|PROT_WRITE, MAP_SHARED,vfd, vbuf.m.offset);

		//把拿出来的缓冲区放回队列
		ret = ioctl(vfd, VIDIOC_QBUF, &vbuf);
		if(ret < 0)
		{
			perror("qbuf error");
			exit(1);
		}
	//}


	//6.开始采集数据
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vfd, VIDIOC_STREAMON, &type);


	//while(1)
	//{
	//7.帧数据出列
		struct v4l2_buffer readbuf;
		readbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		readbuf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(vfd, VIDIOC_DQBUF, &readbuf);
		if(ret < 0)
		{
		perror("dqbuf error");
		exit(1);
		}
	//}
	
	int num=0;
	char buf[100];
	sprintf(buf, "my%d.jpg", num++);
	//保存一帧数据
	FILE *file = fopen(buf, "w+");
	fwrite(mFrames[readbuf.index].start, readbuf.length, 1, file);
	fclose(file);
	
	//把readbuf放回队列
	ret = ioctl(vfd, VIDIOC_QBUF, &readbuf);
	
	//8.停止采集
	ret = ioctl(vfd, VIDIOC_STREAMOFF, &type);

	//9.关闭设备
	close(vfd);
	return 0;
}