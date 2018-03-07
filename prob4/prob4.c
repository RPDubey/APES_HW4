/*******************************************************************************
   @Filename:prob2.c
   @Brief:kThread API and Queus
   use the kthread API to create a kernel module with a second thread that allows the two to communicate via queues (kfifo). The first thread should send information to the second thread on a timed interval through the fifo. The second thread should take data from the kfifoand print it to the kernel logger. The info you should pass should relate to the currently scheduledprocesses in the rbtree. What was the ID and vruntime of the previous, current, and next PID.
   @Author:Ravi Dubey
   @Date:5/2/2018
 ******************************************************************************/
//https://stuff.mit.edu/afs/sipb/contrib/linux/samples/kfifo/record-example.c

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ravi");
MODULE_DESCRIPTION("Kernel Module for kthread practise ");

typedef struct {
        unsigned long long int vrt_c;
        int proc_id_c;
        unsigned long long int vrt_p;
        int proc_id_p;
        unsigned long long int vrt_n;
        int proc_id_n;

}data_struct;

#define FIFO_SIZE (1024)
typedef STRUCT_KFIFO_REC_1 (FIFO_SIZE) myfifo;
static myfifo fifo;
static struct task_struct *thread_st1;
static struct task_struct *thread_st2;


DEFINE_MUTEX(mlock);

static int thread1(void *unused)
{
        printk(KERN_INFO "Entered First Thread\n");

        data_struct data;
//pid and vruntime
        data.proc_id_c = current->pid;
        data.vrt_c = current->se.vruntime;
        data.proc_id_p = list_prev_entry(current,tasks)->pid;
        data.vrt_p = list_prev_entry(current,tasks)->se.vruntime;
        data.proc_id_n = list_next_entry(current,tasks)->pid;
        data.vrt_n = list_next_entry(current,tasks)->se.vruntime;

        while(!kthread_should_stop())
        {

                while(!kfifo_is_full(&fifo)) {

                        mutex_lock(&mlock);
                        kfifo_in(&fifo,(char*)&data,sizeof(data_struct));
                        mutex_unlock(&mlock);
                        printk(KERN_INFO "Data Qued from thread1\n");
                        ssleep(10);

//pid and vruntime
                        data.proc_id_c = current->pid;
                        data.vrt_c = current->se.vruntime;
                        data.proc_id_p = list_prev_entry(current,tasks)->pid;
                        data.vrt_p = list_prev_entry(current,tasks)->se.vruntime;
                        data.proc_id_n = list_next_entry(current,tasks)->pid;
                        data.vrt_n = list_next_entry(current,tasks)->se.vruntime;

                }
                ssleep(1);
        }

        do_exit(0);
        return 0;
}

static int thread2(void *unused)
{

        printk(KERN_INFO "Entered second Thread\n");
        data_struct data;
        while(!kthread_should_stop())
        {
//read from fifo
                while(!kfifo_is_empty(&fifo)) {

                        mutex_lock(&mlock);
                        kfifo_out(&fifo,(char*)&data,sizeof(data_struct));
                        mutex_unlock(&mlock);
                        printk(KERN_INFO "Data DeQued in thread2\n");
                        printk(KERN_INFO "prev    PID & vruntime:%d  %llu\n",
                               ((data_struct*)&data)->proc_id_p,
                               ((data_struct*)&data)->vrt_p);
                        printk(KERN_INFO "current PID & vruntime:%d  %llu\n",
                               ((data_struct*)&data)->proc_id_c,
                               ((data_struct*)&data)->vrt_c);
                        printk(KERN_INFO "next    PID & vruntime:%d  %llu\n",
                               ((data_struct*)&data)->proc_id_n,
                               ((data_struct*)&data)->vrt_n);
                        ssleep(10);
                }
                ssleep(1);
        }

        do_exit(0);
        return 0;
}


static int __init kthreadMod_init(void)
{
        printk(KERN_INFO "Starting kthread Module\n");

        INIT_KFIFO(fifo);//initialize the fifo
        thread_st1 = kthread_create(thread1, NULL, "mythreadone");
        if (thread_st1)
        {
                printk("Thread 1 Created \n");
                wake_up_process(thread_st1);
        }
        else printk(KERN_INFO "Thread 1 failed\n");

        thread_st2 = kthread_create(thread2, NULL, "mythreadtwo");
        if (thread_st2)
        {
                printk("Thread 2 Created\n");
                wake_up_process(thread_st2);
        }
        else
                printk(KERN_INFO "Thread 2  failed\n");
        return 0;
}



static void __exit kthreadMod_cleanup(void)
{
        printk(KERN_INFO "Cleaning up kthread module.\n");

        if (thread_st2 != NULL )
        {
                kthread_stop(thread_st2);
                printk(KERN_INFO "Thread 2 stopped");
        }

        if (thread_st1 != NULL)
        {
                kthread_stop(thread_st1);
                printk(KERN_INFO "Thread 1 stopped");
        }

        kfifo_free(&fifo);
}

module_init(kthreadMod_init);
module_exit(kthreadMod_cleanup);
