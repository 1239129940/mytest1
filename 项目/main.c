#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

#include "font.h"
#include "showbmp.h"

int i=0;

int b=0; 

struct   node 
{
	char name[100];  
	struct sockaddr_in  addr;
};

//初始化Lcd
struct LcdDevice *init_lcd(const char *device)
{
	//申请空间
	struct LcdDevice* lcd = malloc(sizeof(struct LcdDevice));
	if(lcd == NULL)
	{
		return NULL;
	} 

	//1打开设备
	lcd->fd = open(device, O_RDWR);
	if(lcd->fd < 0)
	{
		perror("open lcd fail");
		free(lcd);
		return NULL;
	}
	
	//映射
	lcd->mp = mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcd->fd,0);

	return lcd;
}

void *read_data(void *arg)
{
	
	int *scoket = arg;  
	
	//初始化Lcd
	 struct LcdDevice* lcd = init_lcd("/dev/fb0");
	
	//打开字体	
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	  
	//字体大小的设置
	fontSetSize(f,40);
	
	
	//创建一个画板（点阵图）
	bitmap *bm = createBitmapWithInit(650,270,4,getColor(0,255,255,255)); //也可使用createBitmapWithInit函数，改变画板颜色
	//bitmap *bm = createBitmap(288, 100, 4);
	
	//char buf[] = "晚安";
	
	//读取客户端发送过来的数据 
	while(1)
	{
		
		char buf[1024]={0}; 
		int size=read(*scoket,buf,1024);
			if(size <= 0)
			{
				printf("服务端已经掉线\n");
				close(*scoket); //关闭客户端
				return NULL;
			}
			printf("客户端数据 %s\n",buf);
			
			//判断读取的内容是否为公告
			if(strstr(buf,"公告"))
			{
				printf("接收公告的内容\n");
				
				//创建一个画板（点阵图）
				bitmap *bm = createBitmapWithInit(650,90,4,getColor(0,255,255,255)); //也可使用createBitmapWithInit函数，改变画板颜色
				//将字体写到点阵图上(最后的0可改为800换行)
				fontPrint(f,bm,0,0,buf,getColor(0,0,0,255),0);
				//把字体框输出到LCD屏幕上
				show_font_to_lcd(lcd->mp,150,0,bm);
			}
			
	/*			//初始化Lcd
	 struct LcdDevice* lcd = init_lcd("/dev/fb0");
			
	 //打开字体	
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	  
	//字体大小的设置
	fontSetSize(f,40);
	
	
	//创建一个画板（点阵图）
	bitmap *bm = createBitmapWithInit(650,270,4,getColor(0,255,255,255)); //也可使用createBitmapWithInit函数，改变画板颜色
	//bitmap *bm = createBitmap(288, 100, 4);
	
	//char buf[] = "晚安";
	
	//将字体写到点阵图上(最后的0可改为800换行)
	fontPrint(f,bm,0,0,buf,getColor(0,0,0,0),sizeof(buf)); */

	
	//将字体写到点阵图上(最后的0可改为800换行)
	//fontPrint(f,bm,0,0+40*i,buf,getColor(0,0,0,0),0);
	
	
	if(!strstr(buf,"公告"))
	{
		//将字体写到点阵图上(最后的0可改为800换行)
		fontPrint(f,bm,0,0+40*i,buf,getColor(0,0,0,0),0);
		//把字体框输出到LCD屏幕上
		show_font_to_lcd(lcd->mp,150,190,bm);
	}
	
	i++;      //偏移
	
	//写满屏后，销毁画板，再创建画板
	if(i==7)
	{
		destroyBitmap(bm);//画板需要每次都销毁  
		//创建一个画板（点阵图）
		bitmap *bm = createBitmapWithInit(650,270,4,getColor(0,255,255,255)); //也可使用createBitmapWithInit函数，改变画板颜色
		i=0;
	}

	//把字体框输出到LCD屏幕上
	//show_font_to_lcd(lcd->mp,200,200,bm);

	
	//关闭字体，关闭画板
	//fontUnload(f);  //字库不需要每次都关闭 
	//destroyBitmap(bm);//画板需要每次都销毁  
		
	}

}



void *recv_file(void *arg)
{
	int *scoket = arg;
	
	while(1)
	{
		
		char buf[1024]={0};
		read(*scoket,buf,1024);  
		if(strstr(buf,"send_file"))
		{
			int file_size=0;  
			sscanf(buf,"send_file %d",&file_size);
			printf("对方发送文件是否接收 大小为 %d  1 YES  2 NO\n",file_size);
		
			int a=0;
			scanf("%d",&a);
			
			char file_name[1024]={0};
			printf("请输入另存为名\n");
			scanf("%s",file_name);
			
			
			//应答对方已经接收到文件的大小 
			write(*scoket,"OK",strlen("OK"));
			
			if(a == 1)  //接收文件 
			{
				int fd=open(file_name,O_RDWR|O_CREAT|O_TRUNC,0777);
					if(fd < 0)
					{
						perror("接收失败");
						return 0;
					}

				//接收网络数据写入到本地文件中 
				int  recv_size=0;
				while(1)
				{
					char   recv[4096]={0}; 
					int size = read(*scoket,recv,4096); //读取网络数据 
					recv_size += size;
					printf("当前接收的大小 %d ,总大小 %d\n",size,recv_size);
					write(fd,recv,size); //写入到本地文件中  
					
					if(recv_size == file_size)
					{
						printf("接收完毕\n");
							//应答对方 
						write(*scoket,"END",strlen("END"));
						close(fd); 
						break;
					}
					
					
				}
			}
			else
			{
				continue;
			}
		}
	}	
	}



int main(int argc,char *argv[])
{
	//界面
	showbmp("/root/wyp/project/1.bmp");
	
	//1.创建 TCP  通信协议
       int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
			if(tcp_socket < 0)
			{
				perror("");
				return 0; 
			}
			else
			{
				printf("创建成功\n");
			}
		
			
		int on=1;
		setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEADDR,&on,4);//设置IP复用
		//setsockopt(tcp_socket,SOL_SOCKET,SO_REUSEPORT,&on,4);//设置端口复用
		
		//设置链接的服务器地址信息 
		struct sockaddr_in  addr;  
		addr.sin_family   = AF_INET; //IPV4 协议  
		addr.sin_port     = htons(9301); //端口
		addr.sin_addr.s_addr = inet_addr("192.168.22.173"); //服务器的IP 地址
		//2.链接服务器 
		int ret=connect(tcp_socket,(struct sockaddr *)&addr,sizeof(addr));
		if(ret < 0)
		{
			perror("tcp");
			return 0;
		}
		else
		{
			printf("链接服务器成功\n");
		}
		
		/* //创建一个线程 (读取聊天内容)
		pthread_t  pid;
		pthread_create(&pid,NULL,read_data,&tcp_socket);
		
		//线程(接收文件)
		pthread_t  pid1;
		pthread_create(&pid1,NULL,recv_file,&tcp_socket); */
		
		//发送自己的信息给服务器
		char tmp[1024]={0};
		strcpy(tmp,argv[1]);
		write(tcp_socket,tmp,sizeof(tmp));
		
		
	
	
	 //获取天气
	//1.新建TCP 通信对象 
	 int tcp_socket1 = socket(AF_INET, SOCK_STREAM, 0);

	 //2.链接服务器  
	  //设置服务器的IP地址信息  
	 struct sockaddr_in  addr1;  
	 addr1.sin_family   = AF_INET; //IPV4 协议  
	 addr1.sin_port     = htons(80); //端口 80  ,所有的HTTP 服务器端口都是  80  
	 addr1.sin_addr.s_addr = inet_addr("47.107.155.132"); //服务器的IP 地址信息    
	int ret1=connect(tcp_socket1,(struct sockaddr *)&addr1,sizeof(addr1));
		if(ret1 < 0)
		{
			perror("");
			return 0; 
		}
		else
		{
			printf("链接网络服务器成功\n");
		}
	
	
		//重点！！定制HTTP 请求协议  
		//https://    cloud.qqshabi.cn    /api/hitokoto/hitokoto.php 
char *http = "GET /api.php?key=free&appid=0&msg=天气广州 HTTP/1.1\r\nHost:api.qingyunke.com\r\n\r\n";
	
	
		//发送数据给服务器 
		write(tcp_socket1,http,strlen(http));
	
	
		//获取服务器的信息 
		char  buf1[4096]={0}; 
		int size=read(tcp_socket1,buf1,4096);
		
		char *p=malloc(1024);
		p=strstr(buf1,"广州天气");
		strtok(p,"{");
		
		//printf("size =%d  buf1=%s\n",size,buf1);
		printf("%s\n",p);
		
		
		//写到开发板
		
 		//初始化Lcd
	 struct LcdDevice* lcd = init_lcd("/dev/fb0");
			
	//打开字体	
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	  
	//字体大小的设置
	fontSetSize(f,40);
	
	
	//创建一个画板（点阵图）
	bitmap *bm = createBitmapWithInit(650,100,4,getColor(0,255,255,255)); //也可使用createBitmapWithInit函数，改变画板颜色
	//bitmap *bm = createBitmap(288, 100, 4);
	
	//char buf[] = "晚安";
	
	//将字体写到点阵图上(最后的0可改为800换行)
	fontPrint(f,bm,0,0,p,getColor(0,255,0,0),650);
	
	//把字体框输出到LCD屏幕上
	show_font_to_lcd(lcd->mp,150,90,bm);

	//把字体框输出到LCD屏幕上
	//show_font_to_lcd(lcd->mp,200,200,bm);

	
	//关闭字体，关闭画板
	fontUnload(f);  //字库不需要每次都关闭 
	destroyBitmap(bm);//画板需要每次都销毁   
		
	while(1)
	{
		printf("输入1：发送文件  输入2：聊天\n");
		//int a=0; 
		scanf("%d",&b);
		
		
		if(b == 1)  //文件的发送 
		{
			//线程(接收文件)
			pthread_t  pid1;
			pthread_create(&pid1,NULL,recv_file,&tcp_socket); 
			
			printf("请输入需要发送的文件名\n");
			char file_name[1024];
			scanf("%s",file_name);
		
			
			int fd = open(file_name,O_RDWR);
				if(fd < 0)
				{
					perror("文件不存在");
					break;
				}
			
				
			//获取发送文件的大小 
			int  file_size=lseek(fd,0,SEEK_END);  
				 lseek(fd,0,SEEK_SET);

			char  head[54]={0}; 
			sprintf(head,"send_file %d",file_size);
			write(tcp_socket,head,strlen(head));
			
			//等待对方回应  
			bzero(head,54);
			read(tcp_socket,head,54);
			
			
			if(strstr(head,"OK"))  //对方成功接收到 文件大小
			{
				int  send_size=0;
				while(1)
				{
					//读取文件中的数据  
					char send[4096]={0};
					int size=read(fd,send,4096);
			
					
					printf("发送的大小 %d 总大小 %d\n",size,send_size);
					
					//发送网络中 
					int  s_size=write(tcp_socket,send,size);
						send_size+=s_size;
						if(send_size == file_size)
						{
							printf("发送完毕等待对方接收完毕\n");
							bzero(head,54);
							read(tcp_socket,head,54);
							if(strstr(head,"END"))
							{
								printf("文件传输完毕\n");
								break;
							}
						}
						
						
				}
			}
			
		}
		
		if(b==2)
		{
			 //创建一个线程 (读取聊天内容)
			pthread_t  pid;
			pthread_create(&pid,NULL,read_data,&tcp_socket);
			
			//给其他人发送信息
			while(1)
			{
				char buf[1024]={0};
				scanf("%s",buf);
				
				if(strcmp(buf,"exit")==0)
				{
					break;
				}
				
				//sprintf(retu,"ret %s",argv[1]);
				write(tcp_socket,buf,sizeof(buf));
			}
		}
			
	}

	
		
		/* //给其他人发送信息
		while(1)
		{
			char buf[1024]={0};
			scanf("%s",buf);
			//sprintf(retu,"ret %s",argv[1]);
			write(tcp_socket,buf,sizeof(buf));
		} */
		
	
	//关闭通信 
	close(tcp_socket);
}
