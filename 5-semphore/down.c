

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>


#define PATH "/dev/jsema"




int main(int argc, char *argv[])
{


   int fd;
   int a;
   long ret;
   if( argc != 2)
   {
       printf("usage: down num\n 1 down_interruptible\n 4 down_trylock\n3 down_timeout\n");
       return 0;
   }

   a=atoi(argv[1]);
   printf("cmd = %d\n",a);

   fd = open(PATH, O_RDWR);
   if( fd < 0)
   {
       printf(" can not open %s\n",PATH);
   }

   printf("cmd = %d begin:\n",a);
   ret=ioctl(fd,a,0);
   printf("cmd = %d return ret=%ld\n",a,ret);
   close(fd);

   return 0;
}
