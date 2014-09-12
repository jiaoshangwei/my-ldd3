

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>


static char *name = "jsw";
static struct proc_dir_entry *p = NULL;


static void * j_start(struct seq_file *m, loff_t *off)
{
    if( *off == 0)
    {
        return (void *)0xff;
    }
    else
        return NULL;
}
static void * j_next(struct seq_file *m, void *v, loff_t *off)
{
    return NULL;
}
static int j_show(struct seq_file *m, void *v)
{
    if((uint)v == 0xff)
    {
        seq_printf(m,"%s: This is in proc file :%s\n",__FUNCTION__,name);
    }

    return 0;
}
static void j_stop(struct seq_file *m, void *v)
{
    return;
}

static struct seq_operations j_seq_ops={
    .start = j_start,
    .stop = j_stop,
    .show = j_show,
    .next = j_next,
};

static int proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file,&j_seq_ops);
}

static struct  file_operations j_ops={
    .owner = THIS_MODULE,
    .open = proc_open,
    .release = seq_release,
    .read = seq_read,
    .llseek = seq_lseek,
};

static int proc_init(void)
{
    p = proc_create(name,0644, NULL,&j_ops);

    return 0;
}

static void proc_exit(void)
{
    remove_proc_entry(name,NULL);

    return;
}

module_init(proc_init);
module_exit(proc_exit);
