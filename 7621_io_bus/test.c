
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
 
int main(int argc, char* argv[])
{
    int fd;
    struct pollfd fds[1];
    int ret;
    unsigned int num = 50;
    int num2 = argv[3]-'0';

    fd = open (argv[1], O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(0);
    }

// 通过ioctlk控制输入输出
/*
    if(atoi(argv[2]) == 1)
    {
        ret = ioctl(fd, device_write, &num2);
    }else
    {
        ret = ioctl(fd, device_read, &num);
     	printf("the data is %x", num);
    }
*/
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    while(1)
    {
   	 ret = poll(fds, 1, 2000);
  	 if(ret != -1)
         {
           printf("you shoud to get the data\n");
           ioctl(fd, device_read, &num);
           printf("the data is %x\n", num);
         }
    }
    close(fd);
    printf("the end\n");
    return 0;
}
  
