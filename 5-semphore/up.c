

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>


#define PATH "/dev/jsema"

int main(void)
{

   int fd;
   fd = open(PATH, O_RDWR);
   if( fd < 0)
   {
       printf(" can not open %s\n",PATH);
   }
   ioctl(fd,100,0);

   close(fd);

   return 0;
}
