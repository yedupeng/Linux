#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
 
int main(void)
{
    int fd;
    char sent_buff[128] = "this is a test";
    char read_buff[128] = {};
    int num = 50;
    int num2 = 0;

    fd = open ("/proc/test", O_RDWR);
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

    close(fd);
    printf("the end", sent_buff);

    return 0;
}
