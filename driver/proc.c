#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

char rdata[128] = "hello this is proc file test";
char wdata[128] = {};

static int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "this is proc file open operation");
	return 0;
}

static int release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "this is proc file realse operation");
	return 0;
}

ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	if(count > 127)
		count = 127;
	ssize_t err;
	err = copy_to_user(buff, rdata, count);
	if(err)
	{
		printk(KERN_INFO "error ! copy_to_user error");
		return err;
	}
	printk(KERN_INFO "send data : %s", rdata);
	return count;
}

ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	if(count > 127)
		count = 127;
	ssize_t err;
	err = copy_from_user(wdata, buff, count);
	if(err)
	{
		printk(KERN_INFO "error ! copy_from_user error");
		return err;
	}
	printk(KERN_INFO "Receive data : %s", wdata);
	return count;
}

static const struct  proc_ops proc_file_ops = 
{
	.proc_open 	 = open,		
	.proc_read 	 = read,		
	.proc_write 	 = write,	
	.proc_release = release,	
};

static int __init proc_init(void)
{
	proc_create_data("test", 0644, NULL, &proc_file_ops, NULL);
	return 0;
}

static void __exit proc_exit(void)
{
	remove_proc_entry("test", NULL);
}

module_init(proc_init);
module_exit(proc_exit);

