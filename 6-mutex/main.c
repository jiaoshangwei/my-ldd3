

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>


#define PATH "/dev/jmutex"




int main(int argc, char *argv[])
{


   int fd;
   int a;
   long ret;
   if( argc != 2)
   {
       printf("usage: main num\n 1 mutex_lock_interruptible\n 3 mutex_trylock\n");
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
