#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/nsc_gpio.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include<linux/spinlock.h>
#include "ioctl.h"

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

#define DIDO0 (0xBE000B08)
#define DIDO1 (0xBE000B0C)
#define DIDO2 (0xBE000B10)
#define DIDO3 (0xBE000B14)
#define DIDO4 (0xBE000B18)
#define DIDO5 (0xBE000B1C)
#define DIDO6 (0xBE000B20)
#define DIDO7 (0xBE000B24)

int io_major = 200;
int io_minor = 0;
int numDevice = 1;
struct cdev cdev;
dev_t devno;
struct class *myClass;
struct timer_list mytimer;

#define BUF_LEN 64
static unsigned int g_keys[BUF_LEN];
unsigned int r, w;
#define NEXT_POS(x) ((x+1)%BUF_LEN)

void mytimer_task(struct timer_list  *timer)
{
    mod_timer(timer, jiffies + 100);
    wake_up(&wait_queue);
}

static unsigned int buff_is_empty()
{
    return (r == w);
} 

static unsigned int buff_is_full()
{
    return (r == NEXT_POS(w));
}

static unsigned int get_buff()
{
    unsigned int key = 0;
    if(!buff_is_empty())
    {
        key = g_keys[r];
        r = NEXT_POS(r);
    }
    return key;
}

static unsigned int put_buff(unsigned int key)
{
    if(!buff_is_full())
    {
        g_keys[w] = key;
        w = NEXT_POS(w);
    }
    return w;
}

#define ra_inl(addr) (*(volatile unsigned int *)(addr))

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

    switch(cmd)
    {
       case device_read: 
            put_buff(ra_inl(DIDO0));
            put_buff(ra_inl(DIDO1));
            put_buff(ra_inl(DIDO2));
            put_buff(ra_inl(DIDO3));
            put_buff(ra_inl(DIDO4));
            put_buff(ra_inl(DIDO5));
            put_buff(ra_inl(DIDO6));
            put_buff(ra_inl(DIDO7));

            break;

       case device_write:
            break;
       default:
            break;
    
    }
    return data;
}

unsigned int mem_poll(struct file *filp, poll_table *wait)
{
    struct mem_dev  *dev = filp->private_data; 
    unsigned int mask = 0;
    
    poll_wait(filp, &wait_queue,  wait);
    mask = get_buff();
    return mask;
}

struct file_operations io_fos = 
{
    .owner = THIS_MODULE,
    .open = open,
    .release = release,
    .read = read,
    .write = write,
    .unlocked_ioctl = ioctl,
    .poll = mem_poll
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
    mytimer.expires = jiffies + 100;
    timer_setup(&mytimer, mytimer_task, 0);
    add_timer(&mytimer);
    if(ret < 0)
    {
        printk(KERN_ERR "register_chrdev_region error\n");
    }
    printk(KERN_NOTICE "init\n");
    dev_set();
    cdev_create();
    return 0;
}

static void __exit io_exit(void)
{
    printk(KERN_NOTICE "exit\n");
    cdev_del(&cdev);
    device_destroy(myClass, devno);
    class_destroy(myClass);
    unregister_chrdev_region(devno, numDevice);

}

MODULE_LICENSE("GPL");
module_init(io_init);
module_exit(io_exit);