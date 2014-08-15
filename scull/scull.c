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
#include <linux/fcntl.h>


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

#   define kkdebug(fmt,args...) printk( KERN_DEBUG "scull: " fmt,##args )
//#   define printk(fmt, args...)

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
        void  **data;
        struct scull_qset *next;
};

struct scull_device{
        uint s_qset;
        uint s_quantum;
        struct scull_qset *s_data;
        size_t s_size;
        struct cdev s_cdev;
        int i;
};

struct scull_device *scull_devices = NULL;


static int scull_free(struct scull_device *device)
{
    struct scull_qset *temp=NULL;
    int i;
   
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    while( device->s_data )
    {
        temp = device->s_data;
        device->s_data = temp->next;

       for(i=0; i<device->s_qset; i++)
       {
           kfree( temp->data[i]);
       }

        kfree(temp->data);
        kfree(temp);
    }

    device->s_size = 0;
    return 0;
}

static struct scull_qset * scull_alloc(struct scull_device *device, int n)
{
   int i;
   struct scull_qset *prev,*next;

    printk("%s:%s\n",__FUNCTION__,__TIME__);
   prev = next = device->s_data;
   
   for(i=0; i<=n; i++)
   {
        if(!next)
        {
            next = (struct scull_qset*)kmalloc(sizeof(struct scull_qset),GFP_KERNEL);
            if(!next)
            {
                printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
                return NULL;
            }
            memset(next,0,sizeof(struct scull_qset));

            next->data = kmalloc( sizeof(char *)*device->s_qset,GFP_KERNEL);
            if(!next)
            {
                  
                printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
                return NULL;
            }
            memset(next->data, 0, sizeof(char *)*device->s_qset);

            if(!device->s_data)
            {
                device->s_data = next;
            }
            prev = next;
            next = next->next;
        }
        else
        {
            prev=next;
            next=next->next;
        }
    }

   return prev;
}

static int scull_device_open(struct inode *inode, struct file *filp)
{
    int ret =0;

    struct scull_device *device = container_of(inode->i_cdev,struct scull_device,s_cdev);

    char temp[40];
    dev_t mydev = MKDEV( imajor(inode), iminor(inode));
    print_dev_t(temp,mydev);
    printk("%s:%s\n",__FUNCTION__,temp);
    filp->private_data= (void *)device;

    
    if( (filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        ret= scull_free(device);
    }

    return 0;
}

static int scull_device_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t scull_device_read(struct file *filp, char *buffer, size_t count, loff_t *pos)
{
     
    struct scull_device *device = (struct scull_device *)filp->private_data;
    struct scull_qset *qset;
    loff_t num, offset, qnum, qoffset; 
    size_t temp;

    printk("%s:%s\n",__FUNCTION__,__TIME__);
    if( *pos > device->s_size )
        return 0;

    if( *pos + count > device->s_size)
    {
        count = device->s_size - *pos;
    }

    num = *pos / (device->s_quantum*device->s_qset);
    offset = *pos % (device->s_quantum*device->s_qset);

    qnum= offset / device->s_quantum;
    qoffset = offset % device->s_quantum;

    qset = scull_alloc(device, num);

    if(!qset || !qset->data || !qset->data[qnum] )
    {
        printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
        return -EIO;
    }
   
    if (count > (device->s_quantum - qoffset))
        count = device->s_quantum -qoffset;

    temp=copy_to_user((void *)buffer, (void *)(qset->data[qnum]+qoffset), count);

    *pos+=count;

    return count;

}

static ssize_t scull_device_write(struct file *filp, const char __user *buffer, size_t count, loff_t *pos)
{
    struct scull_device *device = (struct scull_device *)filp->private_data;
    struct scull_qset *qset; 
    size_t temp;
    loff_t num, offset,qnum,qoffset; 

    printk("%s:%s\n",__FUNCTION__,__TIME__);
    num = *pos / (device->s_qset*device->s_quantum);
    offset = *pos % (device->s_qset * device->s_quantum);
    qnum = offset / device->s_quantum;
    qoffset = offset % device->s_quantum;

    qset = scull_alloc(device, num);

    if( !qset || !qset->data)
    {
        printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
        return -EIO;
    }
    if(!qset->data[qnum])
    {
        qset->data[qnum] = kmalloc(device->s_quantum,GFP_KERNEL);
        if(!qset->data[qnum])
        {

            printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
            return -ENOMEM;
        } 
        memset(qset->data[qnum],0,device->s_quantum);
    }

    if( count > device->s_quantum - qoffset)
    {
        count = device->s_quantum - qoffset;
    }

    temp=copy_from_user( (void*)(qset->data[qnum]+qoffset), (void *)buffer, count);

    *pos += count;

    if(*pos > device->s_size)
        device->s_size = *pos;

    return count;
}


static loff_t scull_device_seek(struct file *filp, loff_t pos, int where)
{
    struct scull_device *device = (struct scull_device *)(filp->private_data);

    printk("%s:%s\n",__FUNCTION__,__TIME__);
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
////////////////////////////////////////////////////////////////////
// proc

static void *scull_seq_start( struct seq_file *filp, loff_t *pos)
{
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    if( *pos > scull_devs -1 )
        return NULL;

    return (void *)&scull_devices[*pos];

}

static void *scull_seq_next( struct seq_file *filp, void *buffer, loff_t *pos)
{
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    if(*pos+1 >=  scull_devs -1)
    {
        printk("scull: %s: %d: ERROR!\n", __FUNCTION__,__LINE__);
        return NULL;
    }

    *pos += 1;
    return (void *)&scull_devices[*pos];
}

static int scull_seq_show( struct seq_file *filp, void *buffer)
{
    struct scull_device *dev = (struct scull_device *)buffer;

    printk("%s:%s\n",__FUNCTION__,__TIME__);
    seq_printf(filp,"scull%d: size= %ld\n", dev->i,dev->s_size);

    return 0;
}

static void scull_seq_stop( struct seq_file *filp, void *buffer)
{
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    return;
}

static struct seq_operations scull_seq_operations={
    .start = scull_seq_start,
    .stop = scull_seq_stop,
    .show = scull_seq_show,
    .next = scull_seq_next,
};

static int scull_seq_open(struct inode *inode, struct file *filp)
{
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    return seq_open(filp, &scull_seq_operations);
}

static struct file_operations scull_proc_operations={
    .owner = THIS_MODULE,
    .read = seq_read,
    .llseek = seq_lseek,
    .open = scull_seq_open,
    .release = seq_release,
};

static void scull_proc_install(void )
{
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    proc_create(SCULL_NAME,0,NULL,&scull_proc_operations);
}

static void scull_proc_remove(void)
{
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    remove_proc_entry(SCULL_NAME,NULL);
}

//////////////////////////////////////////////////////////////

static __init int scull_init(void)
{
    int ret =0,i;
    dev_t temp_dev;

    printk("%s:%s\n",__FUNCTION__,__TIME__);
    if(scull_major)
    {
        temp_dev = MKDEV(scull_major, 0);
        ret = register_chrdev_region(temp_dev, scull_devs, SCULL_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&temp_dev ,0, scull_devs, SCULL_NAME);
        scull_major = MAJOR(temp_dev);
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
        scull_devices[i].i = i;

        cdev_init( &(scull_devices[i].s_cdev), &scull_file_operations);
        scull_devices[i].s_cdev.owner = THIS_MODULE;

       ret =  cdev_add( &(scull_devices[i].s_cdev), MKDEV(scull_major,i),1);

       if(ret)
           goto RELEASE_CDEV;
    }

    scull_proc_install();

    return 0;

RELEASE_CDEV: 
    
    printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
    for (i = 0; i<scull_devs; i++)
    {
        cdev_del( &(scull_devices[i].s_cdev));
    }

RELEASE_CHRDEV:

    printk("%s:%d: ERROR!\n",__FUNCTION__,__LINE__);
    unregister_chrdev_region(MKDEV(scull_major,0),scull_devs);
    return -EIO;
}

static void __exit scull_exit(void)
{
    int i=0;
    
    printk("%s:%s\n",__FUNCTION__,__TIME__);
    for( i=0; i<scull_devs; i++)
    {
        cdev_del(&(scull_devices[i].s_cdev)); 
        scull_free(&scull_devices[i]);
    }
    kfree(scull_devices);
    scull_devices = NULL;

    unregister_chrdev_region( MKDEV(scull_major, 0), scull_devs);

    scull_proc_remove();
}

module_init(scull_init);
module_exit(scull_exit);


