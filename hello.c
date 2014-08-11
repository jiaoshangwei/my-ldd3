#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/sched.h>

static int j_int = 0;
static char *j_charp = "NULL";

module_param(j_int,int,0644);
module_param(j_charp,charp, 0644);

MODULE_AUTHOR("jiaoshangwei");
MODULE_DESCRIPTION("this is a hello world!");
MODULE_LICENSE("GPL");

static int __init hello_init(void)
{
    uint temp = KERNEL_VERSION(3,10,0);
    struct task_struct *t = current;
    printk(KERN_INFO "hello_init() j_int=%d, j_charp=%s",j_int,j_charp);
    printk(KERN_INFO "hello_init() LINUX_VERSION_CODE= %d",LINUX_VERSION_CODE);
    printk(KERN_INFO "hello_init() KERNEL_VERSION=%d",temp);

    printk(KERN_INFO "hello_init() t->pid=%d t->comm=%s", t->pid,t->comm);
    return 0;
}

static void __exit hello_exit(void)
{
    struct task_struct *t = current; //current is the same as get_current()
    printk(KERN_INFO "hello_exit() t->pid=%d t->comm=%s",t->pid, t->comm);
    printk(KERN_INFO "hello_exit() ");
    return;
}

module_init(hello_init);
module_exit(hello_exit);
