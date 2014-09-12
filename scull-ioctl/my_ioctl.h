
#ifndef __MY_IOCTL_H__
#define __MY_IOCTL_H__

#ifdef __KERNEL__
    #include <linux/ioctl.h>
#endif

#define IOCTL_JS_TYPE 'J'
#define IOCTL_JS_READ_POINTER _IOR(IOCTL_JS_TYPE,2,int)
#define IOCTL_JS_READ_RETURN _IOR(IOCTL_JS_TYPE,3,int)

#define IOCTL_JS_WRITE_NORMAL _IOW(IOCTL_JS_TYPE,4,int)
#define IOCTL_JS_WRITE_POINTER _IOW(IOCTL_JS_TYPE,5,int)

#define IOCTL_JS_CHANGE_POINTER _IORW(IOCTL_JS_TYPE,6,int)


#endif
