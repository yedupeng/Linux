#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/nsc_gpio.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include<linux/spinlock.h>
#include <linux/delay.h>
#include "ioctl.h"

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);

int io_major = 200;
int io_minor = 0;
int numDevice = 1;
struct cdev cdev;
dev_t devno;
struct class *myClass;

#define CTRL_1 (0xBE000604)
#define POL_1 (0xBE000614)
#define DATA_1 (0xBE000624)
#define DSET_1 (0xBE000634)
#define DCLR_1 (0xBE000644)
#define GPIO_MODE (0xBE000060)

#define _ra_inl(addr) (*(volatile unsigned int *)(addr))
#define _ra_outl(addr, value) (*(volatile unsigned int *)(addr) = (value))

static void __iomem *GPIO_CTRL_1;
static void __iomem *GPIO_POL_1;
static void __iomem *GPIO_DATA_1;
static void __iomem *GPIO_DSET_1;
static void __iomem *GPIO_DCLR_1;
static void __iomem *GPIO_MODE_1;

unsigned int ral_inl(unsigned int addr)
{
    unsigned int retval = _ra_inl(addr);
    printk(KERN_NOTICE "this ral_inl value is %d\n", retval);
    return retval;
}

unsigned int ral_outl(unsigned addr, unsigned int val)
{
    _ra_outl(addr, val);
    printk(KERN_NOTICE "this ral_out value is %d\n", val);
    return val;
}

#define ra_aor(addr, a_mask, o_value) ral_outl(addr, (_ra_inl(addr)&(a_mask)) | (o_value))
#define ra_and(addr, a_mask) ra_aor(addr, a_mask, 0)
#define ra_or(addr, o_value) ra_aor(addr, -1, o_value)

#define SPICTL_SPIENA_HIGH (1<<2)


static int open(struct inode *inode, struct file *filp)
{
    return 0;   
}

static int release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "finish release\n");
    return 0;
}

ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    return count;
}

ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    return count;
}


long int ioctl(struct file *node, unsigned int cmd, unsigned long data)
{
    spin_lock(&test_lock);
    long err;
    void *args = (void *)data;
    int *p = args;

    if(_IOC_TYPE(cmd) != IO_MAGIC) return -ENOTTY;
    if(_IOC_NR(cmd) > device_max) return -ENOTTY;
    if(_IOC_DIR(cmd)&_IOC_WRITE)
        err = !access_ok(_IOC_WRITE, (void __user *)data, _IOC_SIZE(cmd));
    else if(_IOC_DIR(cmd)&_IOC_READ)
        err = !access_ok(_IOC_READ, (void __user *)data, _IOC_SIZE(cmd));
    unsigned int value = 0;

  //  _ra_inl(GPIO_MODE) &= ~(3<<16);
   // _ra_inl(GPIO_MODE) |= 1<<16;

    switch (cmd)
    {
    case device_read:
	_ra_inl(CTRL_1) |= (1<<10);
	_ra_inl(POL_1) &= ~(1<<10);
        _ra_inl(DSET_1) |= (1<<10);
   	msleep(100);
   	
        break;
    
    case device_write:
	_ra_inl(CTRL_1) |= (1<<10);
	_ra_inl(POL_1) &= ~(1<<10);
        _ra_inl(DCLR_1) |= (1<<10);
   	msleep(100);
        break;

    default:
        break;
    }

    spin_unlock(&test_lock);
    return err;
}

struct file_operations io_fos = 
{
    .owner = THIS_MODULE,
    .open = open,
    .release = release,
    .read = read,
    .write = write,
    .unlocked_ioctl = ioctl
};

void dev_set(void)
{
    myClass = class_create(THIS_MODULE, "gpio_io");
    if(IS_ERR(myClass))
    {
        printk(KERN_NOTICE "class_create error\n");
        return;
    }
    device_create(myClass, NULL, devno, NULL, "io_node");
    return;
}

void cdev_create(void)
{
    int err;
    cdev_init(&cdev, &io_fos);
    cdev.owner = THIS_MODULE;
    err = cdev_add(&cdev, devno, numDevice);
    if(err)
    {
        printk(KERN_NOTICE "cdev_add error\n");
    }
}

static int __init io_init(void)
{
    devno = MKDEV(io_major, io_minor);
    int ret;
    ret = register_chrdev_region(devno, numDevice, "gpio_contral");
    if(ret < 0)
    {
        printk(KERN_ERR "register_chrdev_region error\n");
    }
    printk(KERN_NOTICE "init\n");
    dev_set();
    cdev_create();

   // GPIO_DATA_1 = ioremap(DATA_1, 4);
   // GPIO_DSET_1 = ioremap(DSET_1, 4);
  //  GPIO_CTRL_1 = ioremap(CTRL_1, 4);
  //  GPIO_MODE_1 = ioremap(GPIO_MODE, 4);
    return 0;
}

static void __exit io_exit(void)
{
    printk(KERN_NOTICE "exit\n");
    cdev_del(&cdev);
    device_destroy(myClass, devno);
    class_destroy(myClass);
    unregister_chrdev_region(devno, numDevice);

 //   iounmap(GPIO_CTRL_1);
    // iounmap(GPIO_POL_0);
  //  iounmap(GPIO_DATA_1);
  //  iounmap(GPIO_DSET_1);
}

MODULE_LICENSE("GPL");
module_init(io_init);
module_exit(io_exit);
