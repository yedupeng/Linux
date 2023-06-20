
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
    unsigned int num = 50;

    int num2 = argv[3]-'0';
    unsigned int num3 = atoi(argv[3]);

    printf("the data is %d\n", argc-2);

    fd = open (argv[1], O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(0);
    }
    int i;
// 通过ioctlk控制输入输出
    if(atoi(argv[2]) == 1)
    {
        ret = ioctl(fd, device_write, &num2);
    }else if(atoi(argv[2]) == 0)
    {
        ret = ioctl(fd, device_read, &num);
     	printf("the data is %x\n", num);
    }else if(atoi(argv[2]) == 2)
    {
        my_msg.len = atoi(argv[3]);
	ret = ioctl(fd, device_read_e, &my_msg);
        if(ret)
        {
          printf("the ret is %d\n", ret);
        }
        for(i = 0; i<atoi(argv[3]); i++)
            printf("the data is \n");
    }else if(atoi(argv[2]) == 3)
    {
	for(i = 0; i < argc-2; i++)
        {
          my_msg.buff++ = atoi(argv[i+3]);
          printf("the data is %d\n", atoi(argv[i+3]));
	}
       my_msg.len = argc-2;
       ret = ioctl(fd, device_write_e, &my_msg);
    }

    close(fd);
    printf("the end\n");
    return 0;
}
  
