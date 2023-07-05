#include <stdio.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "ioctl.h"

int main(int argc, char* argv[])
{
    int data = 0;
    data = atoi(argv[3]);
    struct pollfd fds[1];
    int fd;
    int ret = 0;
    fd = open(argv[1] ,O_RDWR);
    if(fd < 0)
    {
        perror("error");
        exit(0);
    }

	fds[0].fd = fd;
	fds[0].events = POLLIN;

    if(atoi(argv[2]) == 0)
    {
        ret = ioctl(fd, lightOpen);
        printf("the ret is %d\n", ret);
    }else if(atoi(argv[2]) == 1)
    {
        ret = ioctl(fd, lightClose);
        printf("the ret is %d\n", ret);
    }else if(atoi(argv[2]) == 2)
    {
        while(1)
        {
            ret = poll(fds, 1, 5000);
            if ((fds[0].revents == POLLIN))
            {
                printf("pressed\n");
            }else
            {
                printf("no pressed\n");
            }
        }
    }else if(atoi(argv[2]) == 3)
    {
	ret = ioctl(fd, lightShansuo, &data);
	printf("the ret is %d\n", ret);
    }
}

