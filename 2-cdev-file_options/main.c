

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>


#define PATH "/dev/jdev0"

int main(void)
{

   int fd;
   fd = open(PATH, O_RDWR);
   if( fd < 0)
   {
       printf(" can not open %s\n",PATH);
   }

   char buffer[10];

   write(fd,buffer,10);
   read(fd,buffer,10);

   lseek(fd,10,SEEK_SET);

   ioctl(fd,1,0);

   close(fd);

}
