/*******************************************************************************
   @Filename:problem3.c
   @Brief:kThread Practise (Problem 3)
   This is a kernel module that takes a process ID(current is fine) to investigate the process tree lineage. This module prints information about the parent processes as it traverses backwards up the process tree until it cannot go any further. For eachprocess it goes through, it prints some metrics on that process:
   1)Thread Name 2)Process ID 3)Process Status 4)Number of children 5)Nice Value
   @Author:Ravi Dubey
   @Date:3/5/2018
*******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/list.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ravi");
MODULE_DESCRIPTION("Kernel Module to investigate process tree");

static int mypid = 0;//default value for mypid if nothing is entered from command line
module_param(mypid,int,S_IRUGO);

static int getNumChild(struct task_struct* current_proc){
								int num_child = 0;
								struct list_head* list;
								list_for_each(list,&current_proc->children)
								{
																num_child++;
								}

								return num_child;
}

static int __init pTreeInfo_init(void)
{
								printk(KERN_INFO "Starting pTreeInfo Module\n");
/*kernel stores list of processes in  circular doubly linked list called task_list
 * each element of the Task list is process descriptor of typr struct task_struct*/
								struct task_struct* proc_desc;

/*current macro points to current process descriptor structure
 * on x86, this is done by current_thread_info()->task. Thread
 * info struct lives at the bottom of the stack and has a pointer
 * to task_struct, which is allocated dynamically via slab allocator.*/
								if(mypid == 0) {proc_desc = current;}
//else find the process descriptor for the given pid
								else{
																//get the process structure from PID
																struct pid* pid_struct = find_get_pid(mypid);

																//get task structure from Process structure
																proc_desc = pid_task(pid_struct,PIDTYPE_PID);

								}
//iterate through the lineage
								printk("\nName			PID	State	NICE	#Children\n\n");
								do {

																printk("%-25.15s%-8d%-8ld%-8d%-8d\n", \
																							proc_desc->comm,proc_desc->pid, \
																							proc_desc->state,task_nice(proc_desc), \
																							getNumChild(proc_desc) );
																if(proc_desc == &init_task) break;
																else proc_desc=proc_desc->parent;

								} while(1);



								return 0;
}

static void __exit pTreeInfo_cleanup(void)
{
								printk(KERN_INFO "Cleaning up module.\n");

}

module_init(pTreeInfo_init);
module_exit(pTreeInfo_cleanup);
