

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include "code.h"

MODULE_LICENSE("GPL");

static char *name = "jioctl";

struct jdevice{
    struct cdev j_cdev;
    dev_t j_dev;
};

static struct jdevice *pdevice;

static int j_open(struct inode *inode, struct file *file)
{
    file->private_data = (void *)container_of(inode->i_cdev,struct jdevice,j_cdev); 
    return 0;
}

static int j_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long j_ioctl(struct file *file,uint cmd, ulong arg)
{
    long ret;

    printk("ioctl code: type=%d, num=%d,dir=%d,size=%d\n",_IOC_TYPE(cmd),
           _IOC_NR(cmd),_IOC_DIR(cmd),_IOC_SIZE(cmd));

    switch(cmd){
    case J_IO:
        if( !capable(CAP_SYS_ADMIN))
        {
            ret = -EINVAL;
        }
        else
        {
            printk("J_IO:\n");
            ret = 0;
        }
        break;
    case J_WRITE_VALUE:
        printk("J_WRITE_VALUE:\n");
        ret = arg;
        break;
    case J_WRITE_POINTER:
        printk("J_WRITE_POINTER:\n");
        get_user(ret,(ulong *)arg);
        break;
    case J_READ_POINTER:
        printk("J_READ_POINTER:\n");
        ret = 88;
        put_user(ret,(ulong *)arg);
        break;
    default:
        ret = -ENOTTY;
    };

    return ret;
}

static struct file_operations jops={
    .owner= THIS_MODULE,
    .open = j_open,
    .release = j_release,
    .unlocked_ioctl = j_ioctl,
};

static int ioctl_init(void)
{
    dev_t dev;
   if( alloc_chrdev_region(&dev,0,1,name))
   {
       goto fail_chrdev;
   }

   pdevice = (struct jdevice*)kzalloc(\
              sizeof( struct jdevice), GFP_KERNEL);
   if(!pdevice)
   {
       goto fail_kzalloc;
   }
   pdevice->j_dev = dev;
   cdev_init(&pdevice->j_cdev,&jops);
   pdevice->j_cdev.owner = THIS_MODULE;

   cdev_add(&pdevice->j_cdev,dev,1);

   return 0;

fail_kzalloc:

   unregister_chrdev_region(dev,1);

fail_chrdev:
   return -1;
}

static void ioctl_exit(void)
{
    cdev_del(&pdevice->j_cdev);
    unregister_chrdev_region(pdevice->j_dev,1);
    kfree(pdevice);
}

module_init(ioctl_init);
module_exit(ioctl_exit);

