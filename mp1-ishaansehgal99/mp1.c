#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#include <linux/list.h>
#include <linux/timer.h>

#include <linux/workqueue.h>

#include <linux/time.h>
#include <linux/jiffies.h>

#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

#define FILENAME "status"
#define DIRECTORY "mp1"
#define INTERVAL 5000

static struct proc_dir_entry *proc_dir; 
static struct proc_dir_entry *proc_entry; 
// Timer
static struct timer_list proc_timer;
static void timer_func(struct timer_list * data);
// Lock
static spinlock_t proc_lock;
// Workqueue
static struct work_struct *proc_work;
static struct workqueue_struct *proc_workqueue;
void workqueue_fn(struct work_struct *work); 

typedef struct processes {
   struct list_head list;
   unsigned int pid;
   unsigned long cpu_time;
} processes;

LIST_HEAD(process_list); 

void workqueue_fn(struct work_struct *work){
   processes *tmp, *n; 
   unsigned long flags;
   printk(KERN_INFO "Executing Workqueue Function\n");

   spin_lock_irqsave(&proc_lock, flags);
   list_for_each_entry_safe(tmp, n, &process_list, list) {
      if(get_cpu_use(tmp->pid, &tmp->cpu_time) == -1){
         list_del(&tmp->list);
         kfree(tmp);
      }
   }
   spin_unlock_irqrestore(&proc_lock, flags);

   mod_timer(&proc_timer, jiffies + msecs_to_jiffies(INTERVAL));
}

// User reads from file
static ssize_t mp1_read(struct file *file, char __user *buffer, size_t count, loff_t *data) {
   // implementation goes here
   unsigned long copied = 0; 
   char *res = (char *) kmalloc(count, GFP_KERNEL);
   processes *tmp;
   unsigned long flags;
   
   if (*data > 0) { kfree(res); return 0;}
   spin_lock_irqsave(&proc_lock, flags);
   list_for_each_entry(tmp, &process_list, list) {
      copied += sprintf(res + copied, "%u: %lu\n", tmp->pid, tmp->cpu_time);
   }
   spin_unlock_irqrestore(&proc_lock, flags);
   res[copied] = '\0';

   if (copy_to_user(buffer, res, copied) != 0){
      printk(KERN_ALERT "FAILED TO COPY ALL DATA FROM KERNEL SPACE TO USER SPACE\n");
   }
   kfree(res);
   printk(KERN_ALERT "MP1 READ COMPLETED %lu, %lu\n", copied, count);
   *data = copied;
   return copied;
}

// User writes to file
static ssize_t mp1_write(struct file *file, const char __user *buffer, size_t count, loff_t *data) {
   // implementation goes here
   int pid; 
   unsigned long flags;
   processes *proc = (processes *)kmalloc(sizeof(processes), GFP_KERNEL);
   
   if (kstrtoint_from_user(buffer, count, 10, &pid) != 0){
      printk(KERN_ALERT "FAILED TO PARSE USER PROVIDED PID\n");
      return -1;
   }
   printk(KERN_ALERT "PARSED PID %u\n", pid);

   INIT_LIST_HEAD(&proc->list);
   // printk(KERN_ALERT "PARSED INIT HEAD\n");

   proc->pid = pid; 
   proc->cpu_time = 0; 

   spin_lock_irqsave(&proc_lock, flags);
   list_add(&(proc->list), &process_list);
   // printk(KERN_ALERT "ADDED TO LIST\n");
   spin_unlock_irqrestore(&proc_lock, flags);

   printk(KERN_ALERT "MP1 WRITE COMPLETED\n");

   return count;
}

static void timer_func(struct timer_list *data){
   queue_work(proc_workqueue, proc_work);
}

static const struct proc_ops mp1_file = {
  .proc_read = mp1_read,
  .proc_write = mp1_write, 
};

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   // Insert your code here ...
   proc_dir = proc_mkdir(DIRECTORY, NULL);
   if (proc_dir == NULL) {
      printk(KERN_ALERT "COULD NOT MAKE DIRECTORY\n");
      return -ENOMEM;
   }
   proc_entry = proc_create(FILENAME, 0666, proc_dir, &mp1_file);
   if (proc_entry == NULL) {
      printk(KERN_ALERT "COULD NOT MAKE STATUS FILE\n");
      return -ENOMEM;
   }

   timer_setup(&proc_timer, timer_func, 0);
   mod_timer(&proc_timer, jiffies + msecs_to_jiffies(INTERVAL));

   spin_lock_init(&proc_lock);

   proc_workqueue = create_workqueue("proc_workqueue");
   if (proc_workqueue == NULL){
      printk(KERN_ALERT "COULD NOT MAKE WORKQUEUE\n");
      return -ENOMEM;
   }
   proc_work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
   INIT_WORK(proc_work, workqueue_fn);
   
   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   processes *tmp, *n;
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
   proc_remove(proc_dir);
   del_timer(&proc_timer);
   
   list_for_each_entry_safe(tmp, n, &process_list, list) {
      list_del(&tmp->list);
      kfree(tmp);
   }

   flush_workqueue(proc_workqueue);
   destroy_workqueue(proc_workqueue);

   kfree(proc_work);

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
