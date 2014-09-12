


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/errno.h>


#ifdef HELLO_DEBUG
    #define kdbg(fmt,args...) printk(KERN_ERR fmt,##args)
#else
    #define kdbg(fmt,args...)
#endif


MODULE_AUTHOR("JIAO SHANG WEI");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("this is the third times");
MODULE_VERSION("1.O");

static uint count=1;
static char *name ="hello";

module_param(count,uint,0644);
module_param(name,charp,0644);

static int hello_init(void)
{
    struct task_struct *p = get_current();

    kdbg("%s enter\n", __FUNCTION__);
    kdbg("paramters: count=%d, name=%s\n",count,name);
    kdbg("current'name=%s, current'pid=%d\n",p->comm, p->pid);
    kdbg("kernel version code is %d, make code is %d\n",LINUX_VERSION_CODE, KERNEL_VERSION(3,10,0));

    return 0;
}

static void hello_exit(void)
   {
       kdbg("%s exit\n",__FUNCTION__);
       return;
   }

module_init(hello_init);
module_exit(hello_exit);



