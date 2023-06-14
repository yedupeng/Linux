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
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x21872dcd, "device_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x33874b44, "pv_ops" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xde80cd09, "ioremap" },
	{ 0x57e37143, "device_create" },
	{ 0x9f7184fd, "cdev_add" },
	{ 0x92997ed8, "_printk" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xedc03953, "iounmap" },
	{ 0x1f5ed566, "class_destroy" },
	{ 0xf852ad99, "__class_create" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "BB54F804D828F6F430ED136");
