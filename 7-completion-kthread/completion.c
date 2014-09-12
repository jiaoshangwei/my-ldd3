
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

#define THREAD_NUM 4

static struct task_struct *sthread[THREAD_NUM];
static struct task_struct *wthread;

struct jthread_struct{
    struct completion j_completion;
    int j_flag;
};

static struct jthread_struct *p_struct;

static int sleep_thread(void *data)
{
    struct jthread_struct *p = (struct jthread_struct *)data;
    int ret;

    printk("%d: begin to wait!\n",get_current()->pid);

    do{
        printk("%d: sleep_thread(): wait_for_completion() \n",get_current()->pid);

        ret = wait_for_completion_interruptible(&p->j_completion);

        printk("%d: sleep_thread(): wake up!\n",get_current()->pid);

    }while(!p->j_flag);

    return 5;
}

static int wakeup_thread(void *data)
{
    struct jthread_struct *p =(struct jthread_struct *)data;

    while( !p->j_flag)
    {
        printk("%d: wakeup_thread():--\n",get_current()->pid);
        complete(&p->j_completion);
        msleep(3000);
    }

    return 8;
}

static int completion_init(void)
{
    int i;

    p_struct = kzalloc( sizeof(struct jthread_struct),GFP_KERNEL);
    if(!p_struct)
    {
        goto fail_kzalloc;
    }

    p_struct->j_flag = 0;
    init_completion(&p_struct->j_completion);

    wthread = kthread_run( wakeup_thread,p_struct,"j_wake");
    printk("j_wake started\n");
   
    for( i=0; i<THREAD_NUM; i++)
    {
        sthread[i]=kthread_run(sleep_thread,p_struct,"j_sleep%d",i);
        printk("j_sleep%d started\n",i);
    }

   
    return 0;
fail_kzalloc:
    return -1;
}

static void completion_exit(void)
{
    int i,ret;

    p_struct->j_flag = 1;

    complete_all( &p_struct->j_completion );

    for(i =0;i<THREAD_NUM;i++)
    {
        ret=kthread_stop(sthread[i]);
        printk("j_sleep%d stoped! ret=%d\n",i,ret);
    }

    ret=kthread_stop(wthread);
    printk("j_wake stoped! ret=%d\n",ret);

    kfree(p_struct);
    return;
}

module_init(completion_init);
module_exit(completion_exit);
