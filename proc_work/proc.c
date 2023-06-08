#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

//open operation
static int proc_file_open(struct inode *inode, struct file *file)
{
	printk("Open Operation!\n");
	return 0;
}
//read operation
static ssize_t proc_file_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
	printk("Read Operation!\n");
	return 0;
}
//write opertion
static ssize_t proc_file_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)
{
	printk("Write Operation!\n");
	return len;
}
//release operation
int proc_file_release(struct inode *inode, struct file *file)
{
	printk("Release Operation!\n");
	return 0;
}
static const struct proc_ops proc_file_ops = {
	.proc_open 	 = proc_file_open,		//打开文件操作
	.proc_read 	 = proc_file_read,		//读文件操作
	.proc_write 	 = proc_file_write,		//写文件操作
	.proc_release = proc_file_release,		//关闭文件的释放操作
};
static int proc_file_init(void)
{
	/*
	*创建文件my_proc_file，访问权限为0644，父目录为空，即
	*该文件就在/proc目录下，函数操作集合为proc_file_ops
	*/
	proc_create_data("my_proc_file", 0644, NULL,
		&proc_file_ops, NULL);
	printk("proc_file_init success!\n");
	return 0;
}

static void proc_file_exit(void)
{
	//删除文件my_proc_file
	remove_proc_entry("my_proc_file", NULL);
	printk("proc_file_exit\n");
}

module_init(proc_file_init);
module_exit(proc_file_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("proc Module");
