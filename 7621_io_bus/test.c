
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
    while (1)
    {
        ret = poll(fd, 1, 5000);
        printf("the data is %d", ret);
    }

    close(fd);
    printf("the end\n", sent_buff);
    return 0;
}
