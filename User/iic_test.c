
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include "ioctl.h"
#include <sys/select.h>

struct msg
{
  unsigned int* buff;
  unsigned int len;	
}msg;


int main(int argc, char* argv[])
{
    int fd;
    struct pollfd fds[1];
    int ret;
    struct msg my_msg;
    unsigned int num[3];
    int num2 = argv[3]-'0';
    unsigned int num3[atoi(argv[3])];


    fd = open (argv[1], O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(0);
    }
    int i;
    my_msg.buff = num;
    my_msg.buff[0] = atoi(argv[3]);
    my_msg.buff[1] = atoi(argv[4]);
// 通过ioctlk控制输入输出
    if(atoi(argv[2]) == 1)
    {
        my_msg.buff[2] = atoi(argv[5]);
        ret = ioctl(fd, device_write, &my_msg);
        printf("the data is %d\n", my_msg.buff[2]);
    }else if(atoi(argv[2]) == 0)
    {
        ret = ioctl(fd, device_read, &my_msg);
     	printf("the data is %d\n", my_msg.buff[2]);
    }

    close(fd);
    printf("the end\n");
    return 0;
}
  
