
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
 
int main(void)
{
    int fd;
    char sent_buff[128] = "this is a test";
    char read_buff[128] = {};
    int num = 50;
    int num2 = 0;

    fd = open ("/dev/hello_node", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(0);
    }

    printf("this is the fd : %d \n", fd);
    printf("the size of the receive : %d \n", (int)read(fd, read_buff, sizeof(read_buff)));
    printf("this is the receive data : %s\n", read_buff);
    printf("--------------------------------------------------\n");
    printf("this is the send data : %s\n", sent_buff);
    if(write(fd, sent_buff, strlen(sent_buff)) < 0)
    {
        perror("sent");
    }

    printf("--------------------------------------------------\n");
    int ret;
    ret = ioctl(fd, device_read, &num);
    printf("I send the ioctl data : %d\n", num);
    ret = ioctl(fd, device_write, &num2);
    printf("I receive the ioctl data : %d\n", num2);

    while (1)
    {
        ret = poll(fd, 1, 5000);
        if (ret == 0)
        {
            printf("time out\n");
        }
        else
        {
            printf("the mask is %d\n", ret);
        }
    }

    close(fd);
    printf("the end\n", sent_buff);
    return 0;
}
