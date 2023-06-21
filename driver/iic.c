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
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include<linux/mutex.h>
#include "ioctl.h"

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static DEFINE_MUTEX(mymutex);

#define CTRL_0 (0xBE000600)
#define MODE0 (0xBE000060)
#define POL_0 (0xBE000610)
#define DATA_0 (0xBE000620)
#define DSET_0 (0xBE000630)
#define DCLR_0 (0xBE000640)

#define BUF_LEN 64
#define _ra_inl(addr) (*(volatile unsigned int *)(addr))
#define NEXT_POS(x) ((x+1)%BUF_LEN)
#define NEXT_POS1(x,len) ((x+len)%BUF_LEN)
#define sleep_time 5

int io_major = 200;
int io_minor = 0;
int numDevice = 1;
struct cdev cdev;
dev_t devno;
struct class *myClass;
struct timer_list mytimer;
static unsigned int *lenth;

static unsigned int g_keys[BUF_LEN];
static unsigned int g1_keys[BUF_LEN];
unsigned int r = 0, w = 0, r1 = 0, w1 = 0;

struct msg
{
  unsigned int* buff;
  unsigned int len;	
}msg;

static unsigned int buff_is_empty(int id)
{
    if(id)
    	return (r == w);
    else
	return (r1 == w1);
} 

static unsigned int buff_is_full(int id)
{
    if(id)
    	return (r == NEXT_POS(w));
    else
	return (r1 == NEXT_POS(w1));
}

static unsigned int get_buff(int id)
{
    unsigned int key = 0;
    if(!buff_is_empty(1)&&id == 1)
    {
        mutex_lock(&mymutex);
        key = g_keys[r];
        r = NEXT_POS(r);
        mutex_unlock(&mymutex);
    }else if(!buff_is_empty(0)&&id == 0)
    {
        mutex_lock(&mymutex);
	key = g1_keys[r1];
	r1 = NEXT_POS(r1);
        mutex_unlock(&mymutex);
    }
    return key;
}

static unsigned int put_buff(unsigned int key, int id)
{
    if(!buff_is_full(1)&&id == 1)
    {
        mutex_lock(&mymutex);
        g_keys[w] = key;
        w = NEXT_POS(w);
        mutex_unlock(&mymutex);
        return w;
    }else if(!buff_is_full(0)&&id == 0)
    {
        mutex_lock(&mymutex);
        g1_keys[w1] = key;
        w1 = NEXT_POS(w1);
        mutex_unlock(&mymutex);
        return w1;   
    }
}

#define ra_inl(addr) (*(volatile unsigned int *)(addr))

static int gpio_init(void)
{
    _ra_inl(MODE0) |= 0X01;
    _ra_inl(CTRL_0) |= 1<<1;
    _ra_inl(CTRL_0) |= 1;
    _ra_inl(POL_0) &= ~(1<<1);
    _ra_inl(POL_0) &= ~(1<<0);
    _ra_inl(DSET_0) |= 1;
    _ra_inl(DSET_0) |= 1<<1;
    return 0;
}

static int start(void)
{
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1;
    msleep(sleep_time);

    _ra_inl(DCLR_0) |= 1<<1;
    return 0;
}

static int stop(void)
{
    _ra_inl(DCLR_0) |= 1;
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1;
    msleep(sleep_time);
    return 0;
}

static unsigned int rc_ack(void)
{
    unsigned int ack = 0;
    _ra_inl(CTRL_0) &= ~(1<<0);
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);

    ack = (_ra_inl(DATA_0)) & 0x01;
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(DCLR_0) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);
    msleep(sleep_time);

    return ack;
}


static void sd_ack(void)
{
    _ra_inl(DCLR_0) |= 1;
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(DCLR_0) |= 1<<1;
    return;
}

static void sd_nack(void)
{
    _ra_inl(DSET_0) |= 1;
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    msleep(sleep_time);

    _ra_inl(DCLR_0) |= 1<<1;
    return;
} 

static unsigned int ii2_read(unsigned int inData, unsigned int *outData,int num)
{
    // 7
    unsigned int read_data = 0;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>7)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 6
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>6)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 5
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>5)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 4
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>4)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 3
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>3)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 2
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>2)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 1
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData>>1)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
    // 0
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
    msleep(sleep_time);
    read_data <<= 1;
    read_data |= (inData)&0x01;
    _ra_inl(DCLR_0) |= 1<<1;
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
        _ra_inl(DSET_0) |= 1;
    else
        _ra_inl(DCLR_0) |= 1;
    msleep(sleep_time);
    _ra_inl(DSET_0) |= 1<<1;
}

static unsigned int iic_write(unsigned int indata)
{
    unsigned data_bit, ackSign;
    _ra_inl(DCLR_0) |= 1<<1;

    write_data(indata, 7);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 6);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 5);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 4);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 3);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 2);
    
    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 1);

    msleep(sleep_time);
    msleep(sleep_time);
    _ra_inl(DCLR_0) |= 1<<1;
    write_data(indata, 0);
    printk(KERN_INFO "write data is %d\n",indata);
}


void mytimer_task(struct timer_list  *timer)
{
    mod_timer(&mytimer, jiffies + 400);
    printk(KERN_INFO "time interrupt\n");

    wake_up(&wait_queue);
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
    unsigned int *buff = g_keys;
    unsigned int *buff1 = g1_keys;
    unsigned int max = 0;
    unsigned int indata = 1008611;

    if(_IOC_TYPE(cmd) != IO_MAGIC)
    { 
	return -ENOTTY;
    }
 
    if(_IOC_NR(cmd) > device_max)
    {
	return -ENOTTY;
    }

    if(_IOC_DIR(cmd)&_IOC_WRITE)
    {
        err = !access_ok(_IOC_WRITE, (void __user *)data, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd)&_IOC_READ)
    {
        err = !access_ok(_IOC_READ, (void __user *)data, _IOC_SIZE(cmd));
    }
    if(err)
	    return -EFAULT;
    unsigned int value = 0;
    unsigned int ret = 0;

    void *args = (void *)data;
    unsigned int *p = args; 
    err = copy_from_user(&my_msg, (struct msg __user *)data, sizeof(struct msg));
    
    if(err)
	    return -EFAULT;
    switch(cmd)
    {
       case device_write:
            gpio_init();
            msleep(sleep_time);
            msleep(sleep_time);
            start();
            iic_write(my_msg.buff[0]);
            iic_write(my_msg.buff[1]);
            iic_write(my_msg.buff[2]);
            stop();
            msleep(sleep_time);
            break;
	    
       case device_read:
            gpio_init();
            msleep(sleep_time);
            msleep(sleep_time);
            start();
            iic_write(my_msg.buff[0]);
            iic_write(my_msg.buff[1]);
            iic_write((my_msg.buff[0]| 0x1));
            ii2_read(indata, my_msg.buff+2, 8);
            err = copy_to_user((void *)data, &my_msg, sizeof(struct msg));
            if(err)
            {
               return -EFAULT;
            }
            _ra_inl(DSET_0) |= 1<<1;
            stop();
            msleep(sleep_time);
            break;
       
       case device_read_e:
            break;       

       case device_write_e:

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
    if(!buff_is_empty(1))
    {
       mask |= POLLIN|POLLRDNORM;
       printk(KERN_NOTICE "the buffer not empty, you should take away the data");
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
    return 0;
}

static void __exit io_exit(void)
{
    printk(KERN_NOTICE "exit\n");
    cdev_del(&cdev);
    device_destroy(myClass, devno);
    class_destroy(myClass);
    del_timer(&mytimer);
    unregister_chrdev_region(devno, numDevice);

}

MODULE_LICENSE("GPL");
module_init(io_init);
module_exit(io_exit);
