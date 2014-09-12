
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#ifdef DEV_DEBUG
    #define kdbg(aaa,bbb...) printk(KERN_ERR "%d: " aaa,__LINE__,##bbb)
#else
    #define kdbg(aaa,bbb...)
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("this is for cdev");
MODULE_VERSION("2.0");
MODULE_AUTHOR("jiao shang wei");

static uint dev_size = 4096;
static uint dev_major = 0;
static char *dev_name="jdev";

module_param(dev_size,uint,0644);
module_param(dev_major,uint,0644);
module_param(dev_name,charp,0644);

struct jdevice{
       uint j_size;
       dev_t j_dev;
       struct cdev j_cdev;
       char* j_buffer;
};

static struct jdevice *jdev=NULL;

static int j_open(struct inode *inode, struct file *filp);
static int j_release(struct inode *inode,struct file *file);
static ssize_t j_read(struct file *filp, char __user *buffer, size_t len, loff_t *pos);
static ssize_t j_write(struct file *filp, const char __user *buffer, size_t len, loff_t *pos);
static long j_ioctl(struct file *filp, uint cmd, ulong arg);
static loff_t j_seek(struct file *filp, loff_t off, int whence);

static struct file_operations joperations={
    .owner = THIS_MODULE,
    .open = j_open,
    .release = j_release,
    .write = j_write,
    .read = j_read,
    .llseek = j_seek,
    .unlocked_ioctl = j_ioctl,
};


static int j_open(struct inode *inode, struct file *filp)
{
    struct jdevice *temp;

    kdbg("%s\n",__FUNCTION__);

    kdbg("inode major = %d, minor = %d\n", imajor(inode), iminor(inode));

    filp->private_data = (void *)container_of(inode->i_cdev, struct jdevice, j_cdev);

    temp = (struct jdevice *) filp->private_data;

    kdbg("jdev:size=%d, buffer_addr=%p\n",temp->j_size,temp->j_buffer);

    return 0;
}
    
static int j_release( struct inode *inode, struct file *filp)
{

    kdbg("%s\n",__FUNCTION__);
    return 0;
}

static ssize_t j_read( struct file *filp, char __user *buff, size_t len, loff_t *pos)
{
    struct jdevice *tmp;
    int ret;

    kdbg("%s: pos=%lld, len=%ld\n",__FUNCTION__,filp->f_pos, len);

    tmp=(struct jdevice *)filp->private_data;

    if( filp->f_pos >= tmp->j_size)
    {
        return 0;
    }

    if( filp->f_pos + len > tmp->j_size)
    {
        len = tmp->j_size - filp->f_pos;
    }

    ret = copy_to_user(buff,tmp->j_buffer+filp->f_pos, len);

    filp->f_pos = filp->f_pos + len - ret;
        
    *pos = filp->f_pos;

    return len - ret;

}

static ssize_t j_write( struct file *filp, const char __user *buffer, size_t len, loff_t *pos)
{

    struct jdevice *tmp = (struct jdevice *)filp->private_data;
    int ret;

    kdbg("%s: pos=%lld, len=%ld\n",__FUNCTION__,filp->f_pos, len);

    if( filp->f_pos >= tmp->j_size)
    {
        return -EIO;
    }

    if( filp->f_pos + len > tmp->j_size)
    {
        len = tmp->j_size - filp->f_pos;
    }

    ret = copy_from_user(tmp->j_buffer+filp->f_pos,buffer,len);

    filp->f_pos = filp->f_pos + len - ret;
    
    *pos = filp->f_pos;
    return len - ret;
}


static long j_ioctl( struct file *filp, uint cmd, ulong arg)
{
    kdbg("%s: cmd=%d, arg=%ld\n",__FUNCTION__,cmd,arg);
    return 5;
}

static loff_t j_seek( struct file *filp, loff_t off, int whence)
{

    struct jdevice *tmp = (struct jdevice *)(filp->private_data);

    kdbg("%s: off= %lld, whence=%d\n",__FUNCTION__,off, whence);
    
    switch(whence){
    case SEEK_SET:
        filp->f_pos = off;
        break;
    case SEEK_CUR:
        filp->f_pos += off;
        break;
    case SEEK_END:
        filp->f_pos = tmp->j_size + off;
    };

    if(filp->f_pos > tmp->j_size)
    {
        filp->f_pos = tmp->j_size;
    }

    if(filp->f_pos < 0)
    {
        filp->f_pos = 0;
    }

    return filp->f_pos;
}

static int jdev_init(void)
{
    dev_t dev;

    kdbg("%s enter:\n",__FUNCTION__);

    kdbg("kernel version code is %d, 3.10.0 code is %d\n",LINUX_VERSION_CODE,KERNEL_VERSION(3,10,0)); 

    if( dev_major == 0)
    {
        if( alloc_chrdev_region(&dev,0,1,dev_name) )
        {
            kdbg("%s: alloc_chrdev_region() failed!\n",__FUNCTION__);
            goto fail_alloc;
        }
    }
    else
    {
        if( register_chrdev_region(dev,1,dev_name) )
        {
            kdbg("%s: register_chrdev_region() failed\n",__FUNCTION__);
            goto fail_register;
        }
    }

    kdbg("%s major=%d, minor=%d\n",__FUNCTION__,MAJOR(dev),MINOR(dev));

    jdev= (struct jdevice*)kzalloc( sizeof(struct jdevice),GFP_KERNEL);
    if(!jdev)
    {
        kdbg("%s: kzalloc failed!\n",__FUNCTION__);
        goto fail_alloc_dev;
    }

    jdev->j_buffer =(char *)kzalloc( dev_size,GFP_KERNEL);
    if(!jdev->j_buffer)
    {
        kdbg("%s: kzalloc failed!\n",__FUNCTION__);
        goto fail_alloc_buf;
    }

    jdev->j_size = dev_size;
    jdev->j_dev = dev;
    cdev_init(&jdev->j_cdev,&joperations);
    jdev->j_cdev.owner = THIS_MODULE;

    if( cdev_add(&jdev->j_cdev,dev,1) )
    {
        kdbg("%s: cdev_add() failed\n",__FUNCTION__);
        goto fail_cdev_add;
    }

    return 0;

fail_cdev_add:
    
    kfree(jdev->j_buffer); 

fail_alloc_buf:

    kfree(jdev); 
    jdev=NULL;

fail_alloc_dev:

    unregister_chrdev_region(dev,1);
    
fail_register:
fail_alloc:

    return -1;
}


static void jdev_exit(void)
{

    dev_t dev = jdev->j_dev;

    kdbg("%s \n", __FUNCTION__);
    
    cdev_del(&jdev->j_cdev);

    kfree(jdev->j_buffer);
    kfree(jdev);

    unregister_chrdev_region(dev,1);
    return ;

}

module_init(jdev_init);
module_exit(jdev_exit);
