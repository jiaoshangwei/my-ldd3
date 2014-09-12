#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "my_ioctl.h"

int main(void)
{
    int fd = open("/dev/scull",O_RDWR);

    int ret;
    int a; 
/*
    ret = ioctl(fd, IOCTL_JS_WRITE_NORMAL, 0xaa );

    printf("read normal: 0xaa %x\n",ret);

    a=0xaa;
    ret = ioctl(fd, IOCTL_JS_WRITE_POINTER, &a);
    printf("read pointer: 0xaa  %x\n", a);
*/
    char buf[20];
    printf("read() \n");
    ret = read(fd,buf,20);
    if( ret )
    {
        printf("read error\n");
    }
    printf("write() \n");
    ret = write(fd,buf,3);
    if( ret )
    {
        printf("write error\n");
    }
    close(fd);

    return 0;

}
