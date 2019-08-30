#include "myhead.h"

int card_time(void)
{
	time_t timep;  
	struct tm *p_tm;  
	timep = time(NULL);  
	p_tm = gmtime(&timep); /*获取GMT时间*/  
	printf("%d:%d:%d\n", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec); 
	int number_s = p_tm->tm_hour*3600+p_tm->tm_min*60+p_tm->tm_sec;
    	printf("一共有%d秒。\n", number_s);
	
	return number_s;
}