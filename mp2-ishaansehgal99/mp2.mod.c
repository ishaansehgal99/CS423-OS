#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb7afd9a7, "module_layout" },
	{ 0x24f36513, "proc_remove" },
	{ 0xde395daf, "kmem_cache_destroy" },
	{ 0x2fda72e1, "kthread_stop" },
	{ 0x51b687ac, "kthread_create_on_node" },
	{ 0x4bf0bb98, "kmem_cache_create" },
	{ 0x22d01275, "proc_create" },
	{ 0x1dcdf702, "proc_mkdir" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x11ba74c3, "kmem_cache_free" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0xa0277b85, "kmem_cache_alloc" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x2d5f69b3, "rcu_read_unlock_strict" },
	{ 0xe91dfea8, "pid_task" },
	{ 0xf7c8560, "find_vpid" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x37a0cba, "kfree" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x30890f04, "wake_up_process" },
	{ 0x37885a1b, "sched_setattr_nocheck" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x1000e51, "schedule" },
	{ 0x92997ed8, "_printk" },
	{ 0x9ab43ad6, "current_task" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C57E4FA101AD253A5452E87");
