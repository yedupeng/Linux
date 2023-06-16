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
#include<linux/spinlock.h>
#include "ioctl.h"

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

// 定义寄存器
#define DIDO0 (0xBE000B08)
#define DIDO1 (0xBE000B0C)
#define DIDO2 (0xBE000B10)
#define DIDO3 (0xBE000B14)
#define DIDO4 (0xBE000B18)
#define DIDO5 (0xBE000B1C)
#define DIDO6 (0xBE000B20)
#define DIDO7 (0xBE000B24)

#define CTRL_1 (0xBE000604)
#define POL_1 (0xBE000614)
#define DATA_1 (0xBE000624)
#define DSET_1 (0xBE000634)
#define DCLR_1 (0xBE000644)

// 对寄存器操作函数
#define _ra_inl(addr) (*(volatile unsigned int *)(addr))

int io_major = 200;
int io_minor = 0;
int numDevice = 1;
struct cdev cdev;
dev_t devno;
struct class *myClass;
struct timer_list mytimer;

// 定义接收发送循环缓冲区
#define BUF_LEN 64
static unsigned int g_keys[BUF_LEN];
static unsigned int g1_keys[BUF_LEN];
unsigned int r = 0, w = 0, r1 = 0, w1 = 0;
#define NEXT_POS(x) ((x+1)%BUF_LEN)

// 判断缓冲区是否为空
static unsigned int buff_is_empty(int id)
{
    if(id)
    	return (r == w);
    else
	return (r1 == w1);
} 

// 判断缓冲区是否满
static unsigned int buff_is_full(int id)
{
    if(id)
    	return (r == NEXT_POS(w));
    else
	return (r1 == NEXT_POS(w1));
}

// 从缓冲区中获取数据
static unsigned int get_buff(int id)
{
    unsigned int key = 0;
    if(!buff_is_empty(1)&&id == 1)
    {
        key = g_keys[r];
        r = NEXT_POS(r);
    }else if(!buff_is_empty(0)&&id == 0)
    {
	key = g1_keys[r1];
	r1 = NEXT_POS(r1);
    }
    return key;
}

// 存储数据到缓冲区中
static unsigned int put_buff(unsigned int key, int id)
{
    if(!buff_is_full(1)&&id == 1)
    {
        g_keys[w] = key;
        w = NEXT_POS(w);
        return w;
    }else if(!buff_is_full(0)&&id == 0)
    {
        g1_keys[w1] = key;
        w1 = NEXT_POS(w1);
        return w1;   
    }
}

#define ra_inl(addr) (*(volatile unsigned int *)(addr))

// 定时器触发函数，用于获取总线上DIDO0~7各寄存器的值，同时输出输出缓冲区中的数据
void mytimer_task(struct timer_list  *timer)
{
    mod_timer(&mytimer, jiffies + 400);
    printk(KERN_INFO "time interrupt\n");
    wake_up(&wait_queue);
    if(!buff_is_empty(0))
    {
       
       unsigned int key = get_buff(0);
       if(key)
       {
       	 _ra_inl(CTRL_1) |= (1<<10);
	 _ra_inl(POL_1) &= ~(1<<10);
         _ra_inl(DSET_1) |= (1<<10);
        }else
        {
	 _ra_inl(CTRL_1) |= (1<<10);
	 _ra_inl(POL_1) &= ~(1<<10);
         _ra_inl(DCLR_1) |= (1<<10);
        }
    }else
    {
      	_ra_inl(CTRL_1) |= (1<<10);
	_ra_inl(POL_1) &= ~(1<<10);
        _ra_inl(DCLR_1) |= (1<<10);
    }
    put_buff(ra_inl(DIDO0), 1);
   // printk(KERN_NOTICE "the DIDO0 data is %x\n", ra_inl(DIDO0));
    put_buff(ra_inl(DIDO1), 1);
  //  printk(KERN_NOTICE "the DIDO1 data is %x\n", ra_inl(DIDO1));
    put_buff(ra_inl(DIDO2), 1);
   // printk(KERN_NOTICE "the DIDO2 data is %x\n", ra_inl(DIDO2));
    put_buff(ra_inl(DIDO3), 1);
    //printk(KERN_NOTICE "the DIDO3 data is %x\n", ra_inl(DIDO3));
    put_buff(ra_inl(DIDO4), 1);
    //printk(KERN_NOTICE "the DIDO4 data is %x\n", ra_inl(DIDO4));
    put_buff(ra_inl(DIDO5), 1);
    //printk(KERN_NOTICE "the DIDO5 data is %x\n", ra_inl(DIDO5));
    put_buff(ra_inl(DIDO6), 1);
    //printk(KERN_NOTICE "the DIDO6 data is %x\n", ra_inl(DIDO6));
    put_buff(ra_inl(DIDO7), 1);
    //printk(KERN_NOTICE "the DIDO7 data is %x\n", ra_inl(DIDO7));
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

// 用于用户选择接收还是发送
long int ioctl(struct file *node, unsigned int cmd, unsigned long data)
{
    spin_lock(&test_lock);
    long err;
    void *args = (void *)data;
    unsigned int *p = args;
    

    if(_IOC_TYPE(cmd) != IO_MAGIC) return -ENOTTY;
    if(_IOC_NR(cmd) > device_max) return -ENOTTY;
    if(_IOC_DIR(cmd)&_IOC_WRITE)
        err = !access_ok(_IOC_WRITE, (void __user *)data, _IOC_SIZE(cmd));
    else if(_IOC_DIR(cmd)&_IOC_READ)
        err = !access_ok(_IOC_READ, (void __user *)data, _IOC_SIZE(cmd));
    unsigned int value = 0;
    unsigned int receive_data;

    switch(cmd)
    {
       case device_read: 
	    receive_data = get_buff(1);
            put_user(receive_data, p);
            break;
	    
       case device_write:
            put_buff(*p, 0);
            printk(KERN_NOTICE "the input data is %d\n", *p);
            break;

       default:
            break;
    
    }
    spin_unlock(&test_lock);
    return data;
}

// 提醒用户接收缓冲区非空，请尽快拿走数据
unsigned int mem_poll(struct file *filp, poll_table *wait)
{
    struct mem_dev  *dev = filp->private_data; 
    unsigned int mask = 0;
    
    if(!buff_is_empty(1))
    {
       mask = POLLIN;
       printk(KERN_NOTICE "the buffer not empty, you should take away the data\n");
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
    init_timer(&mytimer);
    mytimer.data = 250;
    mytimer.function = mytimer_task;
    mytimer.expires = jiffies + 1000;
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
    del_timer(&mytimer);
    unregister_chrdev_region(devno, numDevice);

}

MODULE_LICENSE("GPL");
module_init(io_init);
module_exit(io_exit);
