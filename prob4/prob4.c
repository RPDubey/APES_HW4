/*******************************************************************************
   @Filename:prob2.c
   @Brief:kThread API and Queus
   use the kthread API to create a kernel module with a second thread that allows the two to communicate via queues (kfifo). The first thread should send information to the second thread on a timed interval through the fifo. The second thread should take data from the kfifoand print it to the kernel logger. The info you should pass should relate to the currently scheduledprocesses in the rbtree. What was the ID and vruntime of the previous, current, and next PID.
   @Author:Ravi Dubey
   @Date:5/2/2018
 ******************************************************************************/
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>
