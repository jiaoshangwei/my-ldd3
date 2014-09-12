#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <asm/atomic.h>

MODULE_LICENSE("GPL");

static int event_init(void);
static void event_exit(void);

module_init(event_init);
module_exit(event_exit);

struct jevent{
    wait_queue_head_t j_queue;
    int j_flag;
};

static struct task_struct *j_normal[4];
static struct task_struct *j_exclusive[4];
static struct task_struct *j_wakeup;

static struct jevent* pevent;


static int my_thread(void *data,int normal)
{
    struct jevent *p = (struct jevent *)data;
    wait_queue_t wait;
    int ret = 0;

    do{
        init_wait(&wait);

        if( normal)
        {
            printk("%d: normal wait.\n",get_current()->pid);
            prepare_to_wait(&p->j_queue,&wait,TASK_INTERRUPTIBLE);
            schedule();
            printk("%d: normal awaken!\n",get_current()->pid);
        }
        else
        {
            printk("%d: exclusive wait.\n",get_current()->pid);
            prepare_to_wait_exclusive(&p->j_queue,&wait,TASK_INTERRUPTIBLE);
            schedule();
            printk("%d: exclusive awaken.\n",get_current()->pid);
        }

       finish_wait(&p->j_queue,&wait); 

    }while( !p->j_flag);

    return ret;
}

static int exclusive_thread(void *data)
{
    return my_thread(data,0);
}

static int normal_thread(void *data)
{
    return my_thread(data,1);
}

static int wakeup_thread(void *data)
{
    struct jevent *p = (struct jevent *)data;
    
    int i = 0;

    do{
        msleep(2000);
        
        switch(i%4){
        case 0:
            printk("--wake_up()\n");
            wake_up(&p->j_queue);
            break;
        case 1:
            printk("--wake_up_interruptible()\n");
            wake_up_interruptible(&p->j_queue);
            break;
        case 2:
            printk("--wake_up_nr(2)\n");
            wake_up_nr(&p->j_queue,2);
            break;
        case 3:
            printk("--wake_up_interruptible_nr(2)\n");
            wake_up_interruptible_nr(&p->j_queue,2);
            break;
        };

        i++;
        i%=4;

    }while( !p->j_flag);

    return 0;
}

static int event_init(void)
{
    int i = 0;

    pevent = (struct jevent *)kzalloc( sizeof(struct jevent), GFP_KERNEL);
    if(!pevent)
    {
        return -1;
    }

    pevent->j_flag =0;
    init_waitqueue_head(&pevent->j_queue);

    for ( i = 0; i<4; i++)
    {
        j_normal[i] = kthread_run(normal_thread, (void *)pevent,"jnormal-%d",i);
        j_exclusive[i]=kthread_run(exclusive_thread,(void *)pevent,"jexclu-%d",i);
    }

    j_wakeup = kthread_run( wakeup_thread,(void *)pevent,"jwakeup");

    return 0;
}

static void event_exit(void)
{
    int i;

    pevent->j_flag = 1;

    wake_up_all(&pevent->j_queue);
    
    for(i = 0; i<4;i++)
    {
        kthread_stop(j_normal[i]);
        kthread_stop(j_exclusive[i]);
    }

    kthread_stop(j_wakeup);

    kfree(pevent);

    return;
}
