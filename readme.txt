===============================================
本程序实现：
1.建立数据库，刷卡登记入库，摄像头拍照
2.客户端实时显示时间，卡ID号，出入库时间和停车费用
3.照片上传至服务器

===============================================

编译：
服务器： gcc server.c sqlite3.c -o server -ldl -lpthread
客户端： arm-linux-gcc client.c bitmap.c -o client -lpthread -lm



数据库调用函数文件：
sqlite3.c
sqlite3.h


LCD显示字体调用函数文件：
bitmap.c
bitmap.h
font.h
stb_truetype.h
uType.h


硬件设备：USB接口摄像头，RFID模块（连接com2）


运行时先运行服务器在运行客户端


