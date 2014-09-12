

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/sched.h>


struct jdevice{
    dev_t j_dev;
    struct cdev j_cdev;
    struct semaphore j_sema;
};

static struct jdevice *pdevice;
static char *name ="jsema";

static int j_open(struct inode *inode, struct file *filp)
{
    struct jdevice *p = NULL;
    p = (struct jdevice *)container_of(inode->i_cdev, struct jdevice, j_cdev);
    filp->private_data = p;

    return 0;
}

static int j_release( struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t j_write( struct file *file, const char __user *buffer, size_t len, loff_t *pos)
{
    return len;
}

static ssize_t j_read( struct file *file, char __user *buffer, size_t len, loff_t *pos)
{
    return len;
}

//notice ioctl code 2 is used by system.
static long j_ioctl( struct file *file, uint cmd, ulong arg)
{
    int ret=0;
    struct jdevice *p = (struct jdevice *)file->private_data;

    switch(cmd){
    case 1:
        printk("down_interruptible begin:\n");
       ret = down_interruptible(&p->j_sema);
       printk("%d:down_interruptiable() ret=%d\n",get_current()->pid,ret);
       break;
    case 4:
       printk("down_trylock() begin:\n");
       ret = down_trylock( &p->j_sema);
       printk("%d:down_trylock() ret = %d. notice: 0 means has got lock\n",get_current()->pid,ret);
       break;
    case 3:
       printk("down_timeout() begin\n");
       ret = down_timeout(&p->j_sema,5000);
       printk("%d:down_timeout() ret = %d, notice: jiffies\n",get_current()->pid,ret);
       break;
    default:
       printk("up()\n");
       up(&p->j_sema);
       ret =0;
    };

    return ret;
}

static loff_t j_seek( struct file *file, loff_t off, int whence)
{
    return 0;
}

static struct file_operations j_operations={
    .owner = THIS_MODULE,
    .open = j_open,
    .release = j_release,
    .llseek = j_seek,
    .read = j_read,
    .write = j_write,
    .unlocked_ioctl = j_ioctl,
};

static int mysema_init(void)
{
    dev_t dev;
    if( alloc_chrdev_region(&dev,0,1,name))
    {
        printk("%s(): alloc_chrdev_region error!\n",__FUNCTION__);
        goto fail_alloc_region;
    }

    pdevice = (struct jdevice *)kzalloc( sizeof(struct jdevice),GFP_KERNEL);
    if( !pdevice)
    {
        printk("%s(): kzalloc() failed!\n",__FUNCTION__);
        goto fail_alloc_mem;
    }

    cdev_init(&pdevice->j_cdev,&j_operations);
    pdevice->j_cdev.owner = THIS_MODULE;

    sema_init(&pdevice->j_sema,1);

    pdevice->j_dev = dev;

    cdev_add(&pdevice->j_cdev,dev,1);

    return 0;

fail_alloc_mem:

    unregister_chrdev_region(dev,1);

fail_alloc_region:

    return -1;
}

static void mysema_exit(void)
{
    dev_t dev = pdevice->j_dev;
    cdev_del(&pdevice->j_cdev);
    kfree(pdevice);
    unregister_chrdev_region(dev,1);
}

module_init(mysema_init);
module_exit(mysema_exit);




            

