

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "code.h"

#define PATH "/dev/jioctl"

typedef unsigned long ulong;

int main(int argc, char *argv[])
{

   int fd;
   long ret;
   ulong param;

   fd = open(PATH, O_RDWR);
   if( fd < 0)
   {
       printf(" can not open %s\n",PATH);
   }

   printf("J_IO: cmd = %u begin:\n",J_IO);
   ret=ioctl(fd,J_IO,0);
   printf("J_IO: return ret=%ld\n",ret);
  
   param = 5;
   printf("J_WRITE_VALUE: cmd = %u param=%lu,begin:\n",J_WRITE_VALUE,param);
   ret=ioctl(fd,J_WRITE_VALUE,param);
   printf("J_WRITE_VALUE: return ret=%ld\n",ret);
  
   param = 10;
   printf("J_WRITE_POINTER: cmd = %u param=%lu begin:\n",J_WRITE_POINTER,param);
   ret=ioctl(fd,J_WRITE_POINTER,&param);
   printf("J_WRITE_POINTER: return ret=%ld\n",ret);
  

   param = 15;
   printf("J_READ_POINTER: cmd = %u begin:\n",J_READ_POINTER);
   ret=ioctl(fd,J_READ_POINTER,&param);
   printf("J_READ_POINTER: return ret=%ld param = %lu\n",ret,param);

   close(fd);

   return 0;
}
