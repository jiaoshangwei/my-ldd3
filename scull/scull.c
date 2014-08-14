#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>

#ifndef SCULL_MAJOR
#   define SCULL_MAJOR 0
#endif

#ifndef SCULL_DEVS
#   define SCULL_DEVS 4
#endif

#ifndef SCULL_QUANTUM 
#   define SCULL_QUANTUM 4000
#endif

#ifndef SCULL_QSET
#   define SCULL_QSET 1000
#endif

#undef SCULL_NAME
#define SCULL_NAME "scull"

#ifdef __KERNEL__
#   define kdebug(fmt, args...) printk(KERN_DEBUG "scull: " fmt, ## args )
#else
#   define kdebug(fmt, args...)
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiao Shang Wei");
MODULE_VERSION("3.O");
MODULE_DESCRIPTION("write for chapter 3");

static uint scull_major = SCULL_MAJOR;
static uint scull_devs = SCULL_DEVS;
static uint scull_quantum = SCULL_QUANTUM;
static uint scull_qset = SCULL_QSET;

module_param(scull_major, uint, 0644);
module_param(scull_devs, uint, 0644);
module_param(scull_quantum,uint, 0644);
module_param(scull_qset, uint, 0644);

struct scull_qset{
        char *data;
        struct scull_qset *next;
};

struct scull_device{
        uint s_qset;
        uint s_quantum;
        struct scull_qset *s_data;
        size_t s_size;
        struct cdev s_cdev;
};

struct scull_device *scull_devices = NULL;
dev_t scull_dev_n;


static int scull_free(struct scull_device *device)
{
    struct scull_qset *temp=NULL;
   
    while( device->s_data )
    {
        temp = device->s_data;
        device->s_data = temp->next;

        kfree(temp->data);
        kfree(temp);
    }

    return 0;
}

static int scull_alloc(struct scull_device *device)
{
   int i;
   struct scull_qset *temp= NULL;
   
   if(!device)
       return 0;

   if(device->s_data)
       return 0;


   for(i=0; i<scull_qset; i++)
   {
        temp = (struct scull_qset*)kmalloc(sizeof(struct scull_qset),GFP_KERNEL);
        if(!temp)
            goto FREE;

        temp->next = NULL;
        temp->data = (char *)kmalloc(scull_quantum, GFP_KERNEL);
        if(!temp->data)
            goto FREE;

        memset(temp->data,0, scull_quantum);

        if(!device->s_data)
        {
            device->s_data = temp;
        }
        else
        {
            temp->next = device->s_data;
            device->s_data = temp;
        }
   }
   return 0;

FREE:
    scull_free(device);
    return -EIO;
}


static int scull_device_open(struct inode *inode, struct file *filp)
{
    int ret =0;

    struct scull_device *device = container_of(inode->i_cdev,struct scull_device,s_cdev);
    if(!device->s_data)
    {
        ret = scull_alloc(device);
        if(ret)
             return ret;
    }

    filp->private_data= (void *)device;
    return 0;
}

static int scull_device_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t scull_device_read(struct file *filp, char *buffer, size_t count, loff_t *pos)
{
     
    struct scull_device *device = (struct scull_device *)filp->private_data;
    struct scull_qset *qset = device->s_data;
    int i;
    loff_t num, first; 
    size_t temp;

    if( *pos > device->s_size )
        return 0;

    if( *pos + count > device->s_size)
    {
        count = device->s_size - *pos;
    }

    num = *pos / device->s_quantum;
    first = *pos % device->s_quantum;

    if (count > (device->s_size - first))
        count = device->s_size - first;

    for(i=0; i<num; i++)
    {
        qset=qset->next;
    }

    temp=copy_to_user((void *)buffer, (void *)(qset->data+first), count);

    *pos+=count;

    return count;

}

static ssize_t scull_device_write(struct file *filp, const char __user *buffer, size_t count, loff_t *pos)
{
    struct scull_device *device = (struct scull_device *)filp->private_data;
    struct scull_qset *pset = device->s_data;
    int i;
    size_t temp;
    loff_t num, first; 

    if((*pos + count) > (scull_qset * scull_quantum))
        return -EIO;

    num = *pos / scull_quantum;
    first = *pos % scull_quantum;

    if( count > (scull_quantum - first))
        count = scull_quantum-first;

    for(i=0; i<num; i++)
    {
        pset=pset->next;
    }
    temp=copy_from_user( (void*)(pset->data+first), (void *)buffer, count);

    *pos += count;

    if(*pos > device->s_size)
        device->s_size = *pos;

    return count;
}


static loff_t scull_device_seek(struct file *filp, loff_t pos, int where)
{
    struct scull_device *device = (struct scull_device *)(filp->private_data);

    switch (where)
    {
    case SEEK_SET:
        break;
    case SEEK_CUR:
        pos += filp->f_pos;
        break;
    case SEEK_END:
        pos += device->s_size;
        break;
    }

    if(pos < 0)
        pos = 0;
    if(pos > device->s_size)
        pos = device->s_size;

    filp->f_pos= pos;
    return pos;
}

static struct file_operations scull_file_operations={
    .owner = THIS_MODULE,
    .open = scull_device_open,
    .release = scull_device_release,
    .llseek = scull_device_seek,
    .read = scull_device_read,
    .write = scull_device_write
};


static __init int scull_init(void)
{
    int ret =0,i;

    if(scull_major)
    {
        scull_dev_n=MKDEV(scull_major, 0);
        ret = register_chrdev_region(scull_dev_n, scull_devs, SCULL_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&scull_dev_n ,0, scull_devs, SCULL_NAME);
    }

    if(ret)
        return ret;
   

    scull_devices = (struct scull_device *)kmalloc( sizeof(struct scull_device)*scull_devs, GFP_KERNEL);
    if(!scull_devices)
        goto RELEASE_CHRDEV;

    memset(scull_devices, 0 , sizeof(struct scull_device)*scull_devs);

    for(i=0; i<scull_devs; i++)
    {
        scull_devices[i].s_qset = scull_qset;
        scull_devices[i].s_quantum = scull_quantum;
        scull_devices[i].s_size = 0;
        scull_devices[i].s_data = NULL;

        cdev_init( &(scull_devices[i].s_cdev), &scull_file_operations);
        scull_devices[i].s_cdev.owner = THIS_MODULE;

       ret =  cdev_add( &(scull_devices[i].s_cdev), MKDEV(scull_major,i),1);

       if(ret)
           goto RELEASE_CDEV;
    }

    return 0;

RELEASE_CDEV: 
    
    for (i = 0; i<scull_devs; i++)
    {
        cdev_del( &(scull_devices[i].s_cdev));
    }

RELEASE_CHRDEV:

    unregister_chrdev_region(MKDEV(scull_major,0),scull_devs);
    return -EIO;
}

static void __exit scull_exit(void)
{
    int i=0;
    
    for( i=0; i<scull_devs; i++)
    {
        cdev_del(&(scull_devices[i].s_cdev)); 
        scull_free(&scull_devices[i]);
    }
    kfree(scull_devices);
    scull_devices = NULL;

    unregister_chrdev_region( scull_dev_n, scull_devs);
}

module_init(scull_init);
module_exit(scull_exit);


