#include <stdio.h>
#include "alsa/asoundlib.h"

int main(void)
{
	//定义pcm句柄， pcm参数
	snd_pcm_t *handle = NULL;
	snd_pcm_hw_params_t *params = NULL;
	
	
	//2.打开pcm设备---采集方式打开
	snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

	//3 分配参数空间， 设置默认参数
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);

	//设置交错模式
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	//设置采样样本
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U16_LE);
	//设置通道数据
	snd_pcm_hw_params_set_channels(handle, params, 2);
	//设置采样率
	int val = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params, &val,0);

	//把设置好的参数设置到pcm设备
	snd_pcm_hw_params(handle,params);


	//4.分配一个周期空间
	int frame = 0;
	//获取一个周期的帧数
	snd_pcm_hw_params_get_period_size(params, (snd_pcm_uframes_t *)&frame, 0);
	
	int size = frame*4;//计算一个周期所需要的存储空间
	unsigned char *buffer = malloc(size);//分配一个周期的空间

	int t =(int) ((val*10.0)/frame);//计算10秒钟采集的周期数
	
	FILE *file = fopen("./rd.pcm", "r+");//打开文件保存pcm原始数据

	while(t--)
	{
		fread(buffer, size, 1, file);
		snd_pcm_writei(handle, buffer, frame);
	}
	fclose(file);
	
	//销毁pcm句柄
	snd_pcm_drain(handle);
	//关闭pcm设备
	snd_pcm_close(handle);
	//释放堆空间（存储一个周期数据）
	free(buffer);
	return 0;
}	



