#ifndef SHOWBMP_H
#define SHOWBMP_H

#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <linux/fb.h>
#include <linux/input.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <signal.h>

#include <pthread.h>
#include <semaphore.h>
char *p=NULL;

void showbmp(char *tmp)
{
    int bmpfd;
    int lcdfd;
    int i;
    int x,y;
    //定义数组存放读取到的RGB数值
    char bmpbuf[800*480*3];  //char每个数据占1个字节
    //定义int数组存放转换得到的ARGB数据
    int lcdbuf[800*480]; //int每个数据占4个字节
    //定义中间变量存放数据
    int tempbuf[800*480];

    //打开800*480大小的bmp图片
    bmpfd=open(tmp,O_RDWR);
    if(bmpfd==-1)
    {
        printf("打开bmp失败!\n");
        //return -1;
    }
    //打开lcd的驱动
    lcdfd=open("/dev/fb0",O_RDWR);
    if(lcdfd==-1)
    {
        printf("打开lcd失败!\n");
        //return -1;
    }

    //跳过前面没有用的54个字节，从55字节开始读取真实的RGB
    lseek(bmpfd,54,SEEK_SET);

    //读取bmp的rgb数据，从图片的第55个字节开始读取
    read(bmpfd,bmpbuf,800*480*3);
    //bmpbuf[0] --》R
    //bmpbuf[1] --》G
    //bmpbuf[2] --》B
    //把三个字节的RGB--》转换成四个字节的ARGB
    //思路：用位或运算和左移实现数据拼接
    for(i=0; i<800*480; i++)
        lcdbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;
                  // 00[2][1][0]  ABGR
                  // 00[5][4][3]

    //将颠倒的图片翻转过来(x,y)跟(x,479-y)交换即可
    for(x=0; x<800; x++)
    {
        for(y=0; y<480; y++)
            //不应该自己赋值给自己(会覆盖掉一部分像素点)
            //lcdbuf[(479-y)*800+x]=lcdbuf[y*800+x]; //  0-799  800-1599
            tempbuf[(479-y)*800+x]=lcdbuf[y*800+x];
    }
    //把转换得到的ARGB写入到lcd
	p  = mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0); 
    memcpy(p,tempbuf,800*480*4);
	//write(lcdfd,tempbuf,800*480*4);

    //关闭
    close(bmpfd);
    close(lcdfd);
	munmap(p, 800*480*4);
}

#endif // SHOWBMP_H