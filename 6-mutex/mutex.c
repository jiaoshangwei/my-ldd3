


#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
static char *name = "jmutex";

struct jdevice{
    dev_t j_dev;
    struct cdev j_cdev;
    struct mutex j_lock;
};

static struct jdevice *pdevice;

static int j_open(struct inode *inode , struct file *file)
{
    struct jdevice *p = (struct jdevice *)container_of(inode->i_cdev, struct jdevice, j_cdev);
    file->private_data = (void *)p;


    return 0;
}

static int j_release( struct inode *inode, struct file *file)
{

    return 0;
}

static long j_ioctl( struct file *file, uint cmd, ulong arg)
{
    struct jdevice *p = (struct jdevice *)file->private_data;

    int ret =0;
    int locked = 0;

    printk("ioctl: cmd = %u\n",cmd);
    switch(cmd)
    {
    case 1:
        printk("%d: mutex_lock_interruptible() begin\n",get_current()->pid);
        ret=mutex_lock_interruptible(&p->j_lock);
        printk("%d: mutex_lock_interruptible() end. ret = %d\n",get_current()->pid,ret);
        if( !ret )
        {
            printk("%d: mutex is locked\n",get_current()->pid);
            locked =1;
        }
        else
        {
            printk("%d:  mutex is not locked\n",get_current()->pid);
        }
        break;
    case 3:
        printk("%d: mutex_trylock() begin\n",get_current()->pid);
        ret=mutex_trylock(&p->j_lock);
        printk("%d: mutex_trylock() end. ret = %d\n",get_current()->pid,ret);
        if(ret)
        {
            printk("%d: mutex is locked\n",get_current()->pid);
            locked =1;
        }
        else
        {
            printk("%d: mutex is not locked\n",get_current()->pid);
        }
        break;
    default:
        break;
    }

    if( mutex_is_locked(&p->j_lock))
    {
        printk("%d: mutex is locked really!\n",get_current()->pid);
    }
    else
    {
        printk("%d: mutex is not locked really!\n",get_current()->pid);
    }
    
    if( locked )
    {
        msleep(5000);
        printk("%d: mutex unlocked\n",get_current()->pid);
        locked = 0;
        mutex_unlock(&p->j_lock);
    }

    return ret;
}


static struct file_operations jops={
    .owner = THIS_MODULE,
    .open = j_open,
    .release = j_release,
    .unlocked_ioctl = j_ioctl,
};


static int my_init(void)
{
    int ret;
    dev_t dev;
    ret = alloc_chrdev_region(&dev,0,1,name);
    if( ret )
    {
        goto fail_chrdev;
    }

    pdevice = (struct jdevice *)kzalloc( sizeof(struct jdevice),GFP_KERNEL);
    if( !pdevice )
    {
        goto fail_alloc;
    }

    cdev_init(&pdevice->j_cdev,&jops);
    pdevice->j_cdev.owner = THIS_MODULE;

    pdevice->j_dev = dev;
    mutex_init(&pdevice->j_lock);

    cdev_add(&pdevice->j_cdev,dev,1);

    return 0;

fail_alloc:
    unregister_chrdev_region(dev,1);
fail_chrdev:
    return -1;
}

static void my_exit(void)
{
    dev_t dev = pdevice->j_dev;
    cdev_del(&pdevice->j_cdev);
    unregister_chrdev_region(dev,1);
    mutex_destroy(&pdevice->j_lock);
    kfree(pdevice);
    return;
}

module_init(my_init);
module_exit(my_exit);
