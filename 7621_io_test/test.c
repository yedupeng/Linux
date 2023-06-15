
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
    int ret;
    char sent_buff[128] = "this is a test";
    char read_buff[128] = {};
    int num = 50;
    int num2 = 0;

    fd = open ("/dev/hello_node", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(0);
    }
    if(atoi(argv[0]) == 1)
    {
        ret = ioctl(fd, device_write, &num2);
    }else
    {
        ret = ioctl(fd, device_read, &num2);
    }

    close(fd);
    printf("the end\n");
    return 0;
}
