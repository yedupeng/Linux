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
	{ 0xa93d270e, "module_layout" },
	{ 0xa4398079, "cdev_del" },
	{ 0xaa71d79c, "cdev_init" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x21872dcd, "device_destroy" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x57e37143, "device_create" },
	{ 0x24d273d1, "add_timer" },
	{ 0x9f7184fd, "cdev_add" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0x92997ed8, "_printk" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x1f5ed566, "class_destroy" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xf852ad99, "__class_create" },
	{ 0x88db9f48, "__check_object_size" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C76367F61F0E58DD77C7EF0");
