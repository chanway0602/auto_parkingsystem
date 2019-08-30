#include "myhead.h"
#include "sqlite3.h"
/*
	使用sqlite3提供的接口函数操作数据库
	问题一:  38,39双引号嵌套???
	问题二:  二级指针
*/

//定义函数打印查询结果
//int main(int argc,char **argv)
int printresult(void *arg,int col,char **str,char **array)
{
	int i;
	printf("调用查询命令查询结果有%d列\n",col);
	for(i=0; i<col; i++)
	{
		printf("str[%d] is:%s\n",i,str[i]);//*(str+i);
		printf("array[%d] is:%s\n",i,array[i]);//*(array+i);
	}
	return 0;//一定不要漏掉
}
int main()
{
	sqlite3 *mydb;
	int ret;
	//打开数据库文件
	ret=sqlite3_open("./mydata.db",&mydb);
	if(ret!=SQLITE_OK)
	{
		printf("打开数据库文件失败!\n");
		return -1;
	}
	char cmd[128];
	//接着就要新建表格,增删改查
	ret=sqlite3_exec(mydb,"create table if not exists student(name char[10],age int);",NULL,NULL,NULL);
	if(ret!=SQLITE_OK)
	{
		printf("执行命令失败!\n");
		return -1;
	}
	sqlite3_exec(mydb,"insert into student values(\"zhangdan\",18);",NULL,NULL,NULL);	//增加个人信息到表格
	sqlite3_exec(mydb,"insert into student values(\"wangwu\",19);",NULL,NULL,NULL);
	sqlite3_exec(mydb,"select *  from student;",printresult,NULL,NULL);					//查询表格
	//关闭
	sqlite3_close(mydb);
	return 0;
}