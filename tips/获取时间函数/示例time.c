#include "myhead.h"

int main()
{
	time_t mytime; //存放以秒为单位的时间  距离1970-1-1凌晨
	time(&mytime);
	printf("人类看得懂的时间格式是:%s\n",ctime(&mytime));
}