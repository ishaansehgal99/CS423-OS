#define LINUX

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/spinlock.h> 
#include <linux/list.h>
#include <linux/jiffies.h>

#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/sched/types.h>
#include <uapi/linux/sched/types.h>

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#include <linux/timer.h>
#include <linux/time.h>

#include "mp2_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG 1

#define SLEEPING 0
#define READY 1
#define RUNNING 2

#define FILENAME "status"
#define DIRECTORY "mp2"

// Dir
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
// Lock
static spinlock_t proc_lock;
static struct mp2_task_struct* curr;
// Timer
static void timer_func(struct timer_list *timer);
static struct task_struct *dispatch;
// Cache
static struct kmem_cache *mp2_task_cache;

LIST_HEAD(process_list);

// Define our task struct
typedef struct mp2_task_struct {
        struct task_struct *linux_task;
        struct timer_list wakeup_timer;
        struct list_head list;
        pid_t pid;
        unsigned long period_ms;
        unsigned long runtime_ms;
        unsigned long deadline_jiff;
        int state;
        bool started;
} mp2_task_struct;

// Helper for setting the task priority
void set_prio(struct mp2_task_struct *task, int policy, int prio){
        struct sched_attr sparam;
        sparam.sched_priority = prio;
        sparam.sched_policy = policy;
        sched_setattr_nocheck(task->linux_task, &sparam);
}

// Performs admission control - verifies if process can be registered
// Reminder to use integer arithmetic
// Returns 1 if task can be registed, otherwise returns 0
int admission_control(struct mp2_task_struct* task){
        unsigned long util = (task->runtime_ms * 1000000) / task->period_ms;
        mp2_task_struct *tmp, *n;
        unsigned long flags; 
        spin_lock_irqsave(&proc_lock, flags);
        list_for_each_entry_safe(tmp, n, &process_list, list) {
                util += (tmp->runtime_ms * 1000000) / tmp->period_ms;
        }
        spin_unlock_irqrestore(&proc_lock, flags);
        if(util <= 693000) return 1;
        return 0;
}

// Dispatch function - Determines next task to run 
// Two cases for task preemption are 
// if tasks timer expires or yield
// is called from userapp
int dispatch_fn(void *data){
        printk(KERN_ALERT "Dispatch function\n");
        while(true) {
                set_current_state(TASK_INTERRUPTIBLE);
                schedule();
                // Dispatch is woken up from sleep
                printk(KERN_ALERT "Wake up dispatch\n");
                // Exit if asked
                if(kthread_should_stop()) return 0; 
                
                unsigned long flags; 
                spin_lock_irqsave(&proc_lock, flags);
                // Search for task with smallest period / highest priority
                unsigned long min_ms = -1; 
                mp2_task_struct *high_prio_task = NULL; 
                mp2_task_struct *tmp, *n;
                list_for_each_entry_safe(tmp, n, &process_list, list) {
                        if (min_ms == -1 || tmp->period_ms < min_ms){
                                if (tmp->state == READY){
                                        high_prio_task = tmp;
                                        min_ms = tmp->period_ms; 
                                }
                        }
                }
                
                // If no task to schedule
                if (high_prio_task == NULL){
                        if(curr != NULL){
                                set_prio(curr, SCHED_NORMAL, 0);
                        }
                }
                else {
                        if (curr != NULL && high_prio_task->period_ms < curr->period_ms) {
                                // Preempt the current running task 
                                curr->state = READY;
                                set_prio(curr, SCHED_NORMAL, 0); 
                        }
                        // Set this task to running and wake it up
                        high_prio_task->state = RUNNING; 
                        wake_up_process(high_prio_task->linux_task); 
                        set_prio(high_prio_task, SCHED_FIFO, 99); 
                        curr = high_prio_task; 
                }
                spin_unlock_irqrestore(&proc_lock, flags);       
        }
        return 0;
}

// Helper function for searching 
// through our mp2_task_struct 
// based on pid
struct mp2_task_struct *get_mptask_by_pid(pid_t pid){
        mp2_task_struct *tmp, *n;
        list_for_each_entry_safe(tmp, n, &process_list, list) {
                if (tmp->pid == pid) {
                        return tmp;
                }
        }
        return NULL;
}

// Wakeup timer callback function 
// Puts task back in ready state and 
// wakes up dispatcher to schedule task
void timer_func(struct timer_list  *timer){
        printk(KERN_ALERT "Timer went off\n");
        mp2_task_struct *task = from_timer(task, timer, wakeup_timer); 
        pid_t pid = task->pid; 

        unsigned long flags; 
        spin_lock_irqsave(&proc_lock, flags);
        mp2_task_struct *task_wake = get_mptask_by_pid(pid);
        task_wake->state = READY;
        spin_unlock_irqrestore(&proc_lock, flags);

        wake_up_process(dispatch);
}

// When user reads from file
static ssize_t mp2_read(struct file *file, char __user *buffer, size_t count, loff_t *data) {
        unsigned long copied = 0; 
        char *res = (char *) kmalloc(count, GFP_KERNEL);
        unsigned long flags;
        
        if (*data > 0) { kfree(res); return 0;}
        spin_lock_irqsave(&proc_lock, flags);
        mp2_task_struct *tmp;
        list_for_each_entry(tmp, &process_list, list) {
                copied += sprintf(res + copied, "%u: %lu, %lu\n", tmp->pid, tmp->period_ms, tmp->runtime_ms);
        }
        spin_unlock_irqrestore(&proc_lock, flags);
        res[copied] = '\0';

        if (copy_to_user(buffer, res, copied) != 0){
                printk(KERN_ALERT "FAILED TO COPY ALL DATA FROM KERNEL SPACE TO USER SPACE\n");
        }
        kfree(res);
        printk(KERN_ALERT "MP2 READ COMPLETED %lu, %lu\n", copied, count);
        *data = copied;
        return copied;
}

// When user writes to file
static ssize_t mp2_write(struct file *file, const char __user *buffer, size_t count, loff_t *data) {
        char msg[count]; 
        char *split; 
        copy_from_user(msg, buffer, count); 
        msg[count - 1] = '\0';

        if (msg[0] == 'R') {
                // Register a new task 
                printk(KERN_ALERT "Register process\n");
                mp2_task_struct *task = (mp2_task_struct *) kmem_cache_alloc(mp2_task_cache, GFP_KERNEL);
                // Read input
                sscanf(msg+3, "%d, %lu, %lu\n", &task->pid, &task->period_ms, &task->runtime_ms);
                printk(KERN_ALERT "Recieved the following %d, %lu, %lu\n", task->pid, task->period_ms, task->runtime_ms);
                
                // If task doesn't meet admission criteria reject it
                if(!admission_control(task)){
                        printk(KERN_ALERT "Against admission control\n");
                        kmem_cache_free(mp2_task_cache, task);
                        return count;
                }
                // Set initial deadline to 0, update this in yield
                task->deadline_jiff = 0;
                task->state = SLEEPING; 
                task->started = false; 
                timer_setup(&task->wakeup_timer, timer_func, 0);
                task->linux_task = find_task_by_pid(task->pid);
                
                INIT_LIST_HEAD(&task->list);

                unsigned long flags;
                spin_lock_irqsave(&proc_lock, flags);
                list_add(&(task->list), &process_list);
                // printk(KERN_ALERT "ADDED TO LIST\n");
                spin_unlock_irqrestore(&proc_lock, flags);

        } 
        else if (msg[0] == 'Y') {
                // Yield process
                printk(KERN_ALERT "Yield process\n");
                pid_t pid;
                // Read input
                sscanf(msg+3, "%d\n", &pid);
                unsigned long flags; 

                spin_lock_irqsave(&proc_lock, flags);
                mp2_task_struct *yield = get_mptask_by_pid(pid);
                // Update task deadline to reflect next time wakeup time
                if (yield->started == false){
                        yield->deadline_jiff += jiffies + msecs_to_jiffies(yield->period_ms); 
                        yield->started = true;
                } else {
                        yield->deadline_jiff += msecs_to_jiffies(yield->period_ms); 
                }

                // If deadline is not in the future return
                if (yield->deadline_jiff < jiffies){
                        printk(KERN_ALERT "Deadline less than current time\n");
                        return count;
                }
                
                // Update timer
                mod_timer(&(yield->wakeup_timer), yield->deadline_jiff);
          
                yield->state = SLEEPING; 
                spin_unlock_irqrestore(&proc_lock, flags);

                wake_up_process(dispatch);
                // Deprecated:
                // set_task_state(yield->linux_task, TASK_INTERRUPTIBLE);
                
                // Puts the task to sleep and is woken up by dispatch
                set_current_state(TASK_INTERRUPTIBLE);
                schedule(); 
        }
        else if (msg[0] == 'D') {
                // Deregister process
                printk(KERN_ALERT "Deregister process\n");
                pid_t pid; 
                // Read input
                sscanf(msg+3, "%d\n", &pid);
                unsigned long flags;
                
                spin_lock_irqsave(&proc_lock, flags);

                // Delete timer, list entry, and associated struct
                mp2_task_struct *task = get_mptask_by_pid(pid);
                task->state = SLEEPING; 
                del_timer(&task->wakeup_timer);
                list_del(&task->list);
                kmem_cache_free(mp2_task_cache, task);

                // If deregister process is the current process
                // Remove it as the current, and call dispatch
                if (task == curr){
                        curr = NULL; 
                        wake_up_process(dispatch); 
                }

                spin_unlock_irqrestore(&proc_lock, flags); 
        }

        printk(KERN_ALERT "MP2 WRITE COMPLETED\n");
        return count; 
}

// Define file callbacks
static const struct proc_ops mp2_file = {
        .proc_read = mp2_read,
        .proc_write = mp2_write, 
};

// mp2_init - Called when module is loaded
int __init mp2_init(void)
{
        #ifdef DEBUG
        printk(KERN_ALERT "MP2 MODULE LOADING\n");
        #endif
        // Create /proc/mp2/status
        proc_dir = proc_mkdir(DIRECTORY, NULL);
        if (proc_dir == NULL) {
                printk(KERN_ALERT "COULD NOT MAKE DIRECTORY\n");
                return -ENOMEM;
        }
        proc_entry = proc_create(FILENAME, 0666, proc_dir, &mp2_file);
        if (proc_entry == NULL) {
                printk(KERN_ALERT "COULD NOT MAKE STATUS FILE\n");
                return -ENOMEM;
        }

        // Create lock
        spin_lock_init(&proc_lock);

        // Create slab allocator
        mp2_task_cache = KMEM_CACHE(mp2_task_struct, SLAB_PANIC);

        // Create dispatcher thread
        dispatch = kthread_run(dispatch_fn, NULL, "dispatch");

        printk(KERN_ALERT "MP2 MODULE LOADED\n");
        return 0;
}

// mp2_exit - Called when module is unloaded
void __exit mp2_exit(void)
{
        #ifdef DEBUG
        printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
        #endif
        // Stop dispatch thread
        int res;
        res = kthread_stop(dispatch);
        if(!res) {
                printk(KERN_INFO "Dispatcher thread stopped\n");
        }

        // Delete each process list entry
        unsigned long flags; 
        spin_lock_irqsave(&proc_lock, flags);
        mp2_task_struct *tmp, *n;
        list_for_each_entry_safe(tmp, n, &process_list, list) {
                del_timer(&tmp->wakeup_timer);
                list_del(&tmp->list);
                kmem_cache_free(mp2_task_cache, tmp);
        }
        kmem_cache_destroy(mp2_task_cache);
        spin_unlock_irqrestore(&proc_lock, flags);

        // Remove /proc/mp2/status
        proc_remove(proc_dir);

        printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);

