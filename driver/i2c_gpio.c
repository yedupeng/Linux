#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/nsc_gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include "ioctl.h"

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static DEFINE_MUTEX(mymutex);

#define CR_GPIO_BASE       	0xBFBF0200
#define CR_GPIO_CTRL	    (CR_GPIO_BASE + 0x00)
#define CR_GPIO_DATA	    (CR_GPIO_BASE + 0x04)
#define CR_GPIO_ODRAIN      (CR_GPIO_BASE + 0x14)

#define BUF_LEN 64
#define _ra_inl(addr) (*(volatile unsigned int *)(addr))
#define NEXT_POS(x) ((x+1)%BUF_LEN)
#define NEXT_POS1(x,len) ((x+len)%BUF_LEN)
#define sleep_time 10

int io_major = 200;
int io_minor = 0;
int numDevice = 1;
struct cdev cdev;
dev_t devno;
struct class *myClass;
struct timer_list mytimer;
static unsigned int *lenth;
static unsigned int value;
static int irqno;


struct msg
{
  unsigned int* buff;
  unsigned int len;	
}msg;

#define ra_inl(addr) (*(volatile unsigned int *)(addr))

static int gpio_init(void)
{
    // 输出使能
    _ra_inl(CR_GPIO_ODRAIN) |= 1<<1;
    _ra_inl(CR_GPIO_ODRAIN) |= 1<<27;
    // 设置GPIO输出模式
    _ra_inl(CR_GPIO_CTRL) |= 1<<1;
    _ra_inl(CR_GPIO_CTRL) |= 1<<27;
    // 写入数据
    _ra_inl(CR_GPIO_DATA) |= 1<<27;
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    return 0;
}

static int start(void)
{
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<27);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    return 0;
}

static int stop(void)
{
    _ra_inl(CR_GPIO_DATA) &= ~(1<<27);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<27;
    msleep(sleep_time);
    return 0;
}

static unsigned int rc_ack(void)
{
    unsigned int ack = 0;
    _ra_inl(CR_GPIO_CTRL) &= ~(1<<27);
    _ra_inl(CR_GPIO_ODRAIN) &= ~(1<<27);
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);

    ack = (_ra_inl(CR_GPIO_CTRL)&&1<<27);
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    _ra_inl(CR_GPIO_CTRL) |= 1<<27;
    _ra_inl(CR_GPIO_ODRAIN) |= 1<<27;
    msleep(sleep_time);
    msleep(sleep_time);

    return ack;
}


static void sd_ack(void)
{
    _ra_inl(CR_GPIO_DATA) &= ~(1<<27);

    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    return;
}

static void sd_nack(void)
{
    _ra_inl(CR_GPIO_DATA) |= 1<<27;
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    return;
} 

static unsigned int ii2_read(unsigned int inData, unsigned int *outData,int num)
{
    // 7
    unsigned int read_data = 0;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>7)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 6
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>6)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 5
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>5)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 4
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>4)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 3
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>3)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 2
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>2)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 1
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>1)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    // 0
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData)&0x01;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    msleep(sleep_time);
    msleep(sleep_time);
    *outData = read_data;
    if(num >= 8)
        sd_nack();
    else
        sd_ack();
    
    msleep(sleep_time);
    msleep(sleep_time);

    return 0;
}

static unsigned int write_data(unsigned int indata, int i)
{
    msleep(sleep_time);
    unsigned int out;
    out = (indata>>i) & 0x01;
    if(out)
        _ra_inl(CR_GPIO_DATA) |= 1<<27;
    else
        _ra_inl(CR_GPIO_DATA) &= ~(1<<27);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) |= 1<<1;
}

static unsigned int iic_write(unsigned int indata)
{
    unsigned data_bit, ackSign;
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);

    write_data(indata, 7);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 6);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 5);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 4);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 3);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 2);
    
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 1);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(CR_GPIO_DATA) &= ~(1<<1);
    write_data(indata, 0);

    ackSign = rc_ack();
    printk(KERN_INFO "ack data is %d\n",rc_ack);
    return ackSign;
}


void mytimer_task(struct timer_list  *timer)
{
    mod_timer(&mytimer, jiffies + 100);
    wake_up(&wait_queue);
    value = _ra_inl(CR_GPIO_DATA)&(1<<26);
}

static int open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "open success\n");
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
    long err;
    struct msg my_msg;
    unsigned int max = 0;
    unsigned int indata = 1008611;

    if(_IOC_TYPE(cmd) != magic_id)
    { 
	return -ENOTTY;
    }
 
    if(_IOC_NR(cmd) > device_max)
    {
	return -ENOTTY;
    }

    if(_IOC_DIR(cmd)&_IOC_WRITE)
    {
        err = !access_ok((void __user *)data, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd)&_IOC_READ)
    {
        err = !access_ok((void __user *)data, _IOC_SIZE(cmd));
    }
    if(err)
	    return -EFAULT;
    unsigned int ret = 0;

    void *args = (void *)data;
    unsigned int *p = args; 
    err = copy_from_user(&my_msg, (struct msg __user *)data, sizeof(struct msg));
    
    if(err)
	    return -EFAULT;
    switch(cmd)
    {
       case light_open:
            // 点灯
            msleep(sleep_time);
            msleep(sleep_time);
            start();
            // address
            iic_write(78);
            // contral
            iic_write(2);
            // data
            iic_write(239);
            stop();
            msleep(sleep_time);
            break;
	    
       case light_close:
            // 灭灯
            msleep(sleep_time);
            msleep(sleep_time);
            start();
            // address
            iic_write(78);
            // contral
            iic_write(2);
            // data
            iic_write(255);
            stop();
            msleep(sleep_time);
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
    poll_wait(filp, &wait_queue, wait);
    if(!value)
    {
        mask |= POLLIN|POLLRDNORM;
    }
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

void key_init(void)
{
    msleep(sleep_time);
    msleep(sleep_time);
    start();
    // address
    iic_write(78);
    // contral
    iic_write(2);
    // data
    iic_write(1);
    stop();
    msleep(sleep_time);
}

static int __init io_init(void)
{
    devno = MKDEV(io_major, io_minor);
    int ret;
    ret = register_chrdev_region(devno, numDevice, "gpio_contral");
    if(ret < 0)
    {
        printk(KERN_ERR "register_chrdev_region error\n");
        return -1;
    }
    mutex_init(&mymutex);
    gpio_init();
    printk(KERN_NOTICE "init\n");
    dev_set();
    cdev_create();
    key_init();
    return 0;
}

static void __exit io_exit(void)
{
    printk(KERN_NOTICE "exit\n");
    cdev_del(&cdev);
    device_destroy(myClass, devno);
    class_destroy(myClass);
    //del_timer(&mytimer);
    unregister_chrdev_region(devno, numDevice);
}

MODULE_LICENSE("GPL");
module_init(io_init);
module_exit(io_exit);
