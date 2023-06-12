#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/nsc_gpio.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");

void gpio_write(struct file *file, const char __user *data,
		       size_t len, loff_t *ppos)
{
    unsigned m = iminor(file_inode(file));
    struct nsc_gpio_ops *gpioData = file->private_data;
    struct device *dev = gpioData->dev;
    size_t i;
    int err = 0;

    for(i = 0; i < len; i++)
    {
        char c;
        if(get_user(c, data + i))
        {
            return -EFAULT;
        }
        switch (c)
        {
        case '0':
            gpioData->gpio_get(m, 0);
            break;
        
        case '1':
            gpioData->gpio_set(m, 1);
            break;
        
        default:
            printk("end~\n");
            err++;
        }
    }
    if(err)
        return -EINVAL;
    return len;
}

ssize_t gpio_read(struct file *file, char __user * buf,
		      size_t len, loff_t * ppos)
{
    unsigned m = iminor(file_inode(file));
    int value;
    struct nsc_gpio_ops *gpioData = file->private_data;
    value = gpioData->gpio_get(m);
    
    int ret;
    if(value)
        ret = put_user('1', buf);
    else
        ret = put_user('0', buf);
    if(ret)
    {
        return -EFAULT;
    }
    return 1;
}

static int __init gpio_init(void)
{
	printk(KERN_DEBUG " init\n");
	return 0;
}

static void __exit gpio_exit(void)
{
	printk(KERN_DEBUG " cleanup\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

