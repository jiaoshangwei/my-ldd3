
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

#define SCULL_MAJOR 0
#define SCULL_NUM   4
#define SCULL_NAME "scull"
#define SCULL_SET  1000
#define SCULL_BLOCK 4000

static uint scull_major = SCULL_MAJOR;
static uint scull_num = SCULL_NUM;
static uint scull_set = SCULL_SET;
static uint scull_block = SCULL_BLOCK;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiao Shang Wei");
MODULE_VERSION("2.0");
MODULE_DESCRIPTION("This is for scull!");

module_param(scull_major, uint , 0644);
module_param(scull_num, uint, 0644);
module_param(scull_set, uint, 0644);
module_param(scull_block, uint, 0644);
       

struct scull_qset{
    void **data;
    struct scull_qset *next;
};

struct scull_device{
    dev_t dev;

    struct cdev s_cdev;
    struct scull_qset *data;
    uint size;

    struct mutex lock;
};

static struct scull_device *scull_devices;



static struct scull_qset * scull_alloc( struct scull_device *device, uint n)
{
   int i;
   struct scull_qset *cur,*next;
   
   printk(KERN_ERR "%s :enter\n",__FUNCTION__);

   cur = next = device->data;
   for( i=0; i<=n; i++)
   {
       if( !next )
       {
           next = (struct scull_qset *)kmalloc( sizeof(struct scull_qset), GFP_KERNEL);
           if( !next)
           {
                printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
               return NULL;
           }

           memset(next, 0, sizeof(struct scull_qset));

            if( !cur)
            {
                device->data = next;
            }
            else
            {
                cur->next = next;
            }

       }
       cur=next;
       next=next->next;
       printk("scull_alloc:: i= %d, cur = %p, next=%p\n",i,cur,next);
    }

    return cur;
}

static void scull_free( struct scull_device *device)
{
    struct scull_qset *cur;
    int i;
   
    printk(KERN_ERR "%s :enter\n",__FUNCTION__);
   
    while(device->data)
    {
       cur = device->data;
       device->data = cur->next;

       for(i=0; i< scull_set; i++)
       {
           kfree(cur->data[i]);
       }

       kfree(cur);
    }

    device->size = 0;

    return;
}

static int scull_open( struct inode *inode, struct file *file)
{
    uint major, minor;
    struct cdev *p = inode->i_cdev; 
    
    struct scull_device *device = container_of(p,struct scull_device,s_cdev);

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);
   
    if( (file->f_flags & O_ACCMODE) == O_WRONLY)
    {
        scull_free(device);
    }

    major = imajor(inode);
    minor = iminor(inode);
    device->dev = MKDEV(major, minor);

    file->private_data = device;
    
    return 0;
}
    

static int scull_release( struct inode *in__ode, struct file *file)
{
    return 0;
}

static ssize_t scull_read( struct file *file, char *buffer, size_t count, loff_t *pos)
{
    uint qnum, qoff, bnum, boff;
    size_t ret;
    struct scull_qset *set;
    struct scull_device *device = (struct scull_device *)file->private_data;
    int lock_ret;
    char *p;

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);
   
    qnum = *pos / ( scull_set * scull_block);
    qoff = *pos % ( scull_set * scull_block);

    bnum = qoff / scull_block;
    boff = qoff % scull_block;

    if( *pos >= device->size)
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return 0;
    }

    if( *pos + count > device->size)
    {
        count = device->size - *pos;
    }

    lock_ret=mutex_lock_killable(&device->lock);
    if(lock_ret)
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return -EIO;
    }
    set = scull_alloc(device, qnum);
    mutex_unlock(&device->lock);

    if(!set)
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return -EIO;
    }

    if( !set->data )
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return -EIO;
    }

    if( !set->data[bnum] )
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return -EIO;
    }

    if( boff + count > scull_block)
    {
        count = scull_block - boff;
    }

    p=(char *)set->data[bnum];

    ret = copy_to_user( (void *)buffer, (void *)(p+boff), count);

    *pos += count;

    return count;
}


static ssize_t scull_write( struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    uint qnum, qoff, bnum, boff;
    size_t ret;
    struct scull_qset *set;
    struct scull_device *device = (struct scull_device *)file->private_data;
    int lock_ret;
    char *p;

   
    printk(KERN_ERR "%s :enter count=%d, pos=%d\n",__FUNCTION__,count, *pos);
   
    qnum = *pos / ( scull_set * scull_block);
    qoff = *pos % ( scull_set * scull_block);
    bnum = qoff / scull_block;
    boff = qoff % scull_block;
    
    printk(KERN_ERR "qnum=%d, qoff=%d, bnum=%d, boff=%d \n", qnum,qoff,bnum,boff);

    lock_ret = mutex_lock_killable( &device->lock );
    if( lock_ret)
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return -EIO;
    }
    set = scull_alloc(device,qnum);
    mutex_unlock( &device->lock );

    if(!set)
    {
        printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
        return -EIO;
    }

    if( !set->data )
    {
        set->data = kmalloc( sizeof(char *)*scull_set, GFP_KERNEL);

        if( !set->data )
        {
            printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
            return -EIO;
        }

        memset(set->data, 0, sizeof( char *)*scull_set);
    }

    if( !set->data[bnum] )
    {
        set->data[bnum] = (char *)kmalloc( sizeof(char)*scull_block, GFP_KERNEL);
        if( !set->data[bnum])
        {
            printk(KERN_ERR "%s : error! line=%d\n",__FUNCTION__,__LINE__);
            return -EIO;
        }

        memset(set->data[bnum], 0, sizeof(char)*scull_block);
    }

    if( boff + count > scull_block)
    {
        count = scull_block - boff;
    }

    p = (char *)set->data[bnum];
    p += boff;
  
    ret = copy_from_user( (void *)p,(void *)buffer, count);
    printk("scull:copy_from_user(): count=%d, ret=%d \n",count,ret);

    if( *pos + count > device->size)
    {
        device->size = *pos + count;
    }

    *pos += count;

    return count;
}

static loff_t scull_seek( struct file *file, loff_t off, int whence)
{
    loff_t temp= 0;
  
    struct scull_device *device = (struct scull_device *)file->private_data;

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);

    switch (whence){
    case SEEK_SET:
           temp = off;
           break;
    case SEEK_CUR:
           temp = file->f_pos + off;
           break;
    case SEEK_END:
           temp = device->size + off;
    }

   if( temp < 0 )
       temp = 0;

   file->f_pos = temp;

   return temp;
}

static struct file_operations scull_file_operation ={
    .owner = THIS_MODULE,
    .llseek = scull_seek,
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
};

////////////////////////////////////////////////////

static void *scull_start( struct seq_file *m, loff_t *pos)
{

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);
   
    if( *pos >= scull_num )
        return NULL;

    return (void *)&scull_devices[*pos];
}
                   
static void *scull_next( struct seq_file *m, void *v, loff_t *ops)
{

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);
   
    if(*ops +1 >= scull_num)
        return NULL;

    *ops += 1;
    return (void *)&scull_devices[*ops];
}

static int  scull_show( struct seq_file *m, void *v)
{
    struct scull_device *device = (struct scull_device *)v;

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);

    seq_printf(m, "scull[%d]: size=[%d]\n", MINOR(device->dev), device->size);
    return 0;
}

static void scull_stop(struct seq_file *m, void *v)
{
    return;
}

static struct seq_operations scull_proc_operation={
    .start = scull_start,
    .stop = scull_stop,
    .show = scull_show,
    .next = scull_next,
};

static int scull_proc_open( struct inode *inode, struct file *file)
{
    return seq_open(file, &scull_proc_operation );
}

static struct file_operations scull_proc_file_operation={
    .owner = THIS_MODULE,
    .open = scull_proc_open,
    .release = seq_release,
    .read = seq_read,
    .llseek = seq_lseek,
};

static void scull_proc_init(char *name )
{
    proc_create(name ,0,NULL, &scull_proc_file_operation);
}

static void scull_proc_release( char *name)
{
    remove_proc_entry(name, NULL);
}

/////////////////////////////////////////////////////////////

static __init  int scull_init(void)
{
    int i,ret;
    dev_t dev;


    printk(KERN_ERR "%s :enter\n",__FUNCTION__);
    
    if( scull_major )
    {
        dev = MKDEV(scull_major,0);
        ret = register_chrdev_region(dev, scull_num, SCULL_NAME);
        if(ret)
        {
            return -EIO;
        }

    }
    else
    {
        ret = alloc_chrdev_region( &dev, 0, scull_num,SCULL_NAME);
        if(ret)
        {
            return -EIO;
        }

        scull_major = MAJOR( dev);
    }


    scull_devices = (struct scull_device *)kmalloc( sizeof(struct scull_device)*scull_num, GFP_KERNEL);
    if(!scull_devices)
        goto DEV; 

    memset(scull_devices, 0, sizeof(struct scull_device)*scull_num);

    for ( i=0; i< scull_num; i++)
    {
        mutex_init( &scull_devices[i].lock);
        cdev_init( &scull_devices[i].s_cdev,&scull_file_operation);
        scull_devices[i].s_cdev.owner = THIS_MODULE;

        ret = cdev_add( &scull_devices[i].s_cdev, MKDEV(scull_major,i), 1);
        if(ret)
        {
            goto REMOVE;
        }
    }

    scull_proc_init(SCULL_NAME);

    return 0;

REMOVE:
    for( i=0; i< scull_num; i++)
    {
        cdev_del( &scull_devices[i].s_cdev );
    }

    kfree(scull_devices);

DEV:
   unregister_chrdev_region( MKDEV(scull_major,0), scull_num);
   return -EIO;
}

static __exit void scull_exit(void)
{

    int i;

    printk(KERN_ERR "%s :enter\n",__FUNCTION__);

    scull_proc_release(SCULL_NAME);

    for ( i=0; i<scull_num; i++)
    {
        cdev_del( &scull_devices[i].s_cdev );
        scull_free( &scull_devices[i]);
    }

    kfree( scull_devices);

    unregister_chrdev_region(MKDEV(scull_major,0), scull_num);
}

module_init(scull_init);
module_exit(scull_exit);
    
