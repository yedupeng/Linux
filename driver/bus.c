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

#define BUF_LEN 64
#define _ra_inl(addr) (*(volatile unsigned int *)(addr))
#define NEXT_POS(x) ((x+1)%BUF_LEN)
#define NEXT_POS1(x,len) ((x+len)%BUF_LEN)

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

void mytimer_task(struct timer_list  *timer)
{
    mod_timer(&mytimer, jiffies + 400);
    printk(KERN_INFO "time interrupt\n");
    if(!buff_is_empty(0))
    {
       unsigned int key = get_buff(0);
       printk(KERN_INFO "the data %d",key);
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
   printk(KERN_NOTICE "the DIDO0 data is %x\n", ra_inl(DIDO0));
    put_buff(ra_inl(DIDO1), 1);
    printk(KERN_NOTICE "the DIDO1 data is %x\n", ra_inl(DIDO1));
    put_buff(ra_inl(DIDO2), 1);
    printk(KERN_NOTICE "the DIDO2 data is %x\n", ra_inl(DIDO2));
    put_buff(ra_inl(DIDO3), 1);
    printk(KERN_NOTICE "the DIDO3 data is %x\n", ra_inl(DIDO3));
    put_buff(ra_inl(DIDO4), 1);
    printk(KERN_NOTICE "the DIDO4 data is %x\n", ra_inl(DIDO4));
    put_buff(ra_inl(DIDO5), 1);
    printk(KERN_NOTICE "the DIDO5 data is %x\n", ra_inl(DIDO5));
    put_buff(ra_inl(DIDO6), 1);
    printk(KERN_NOTICE "the DIDO6 data is %x\n", ra_inl(DIDO6));
    put_buff(ra_inl(DIDO7), 1);
    printk(KERN_NOTICE "the DIDO7 data is %x\n", ra_inl(DIDO7));

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
    unsigned int buff[64];
    my_msg.buff = buff;
    unsigned int max = 0;

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
    unsigned int value = 0;
    unsigned int receive_data;
    unsigned int ret = 0;
    void *args = (void *)data;
    unsigned int *p = args; 
    
    if(err)
	return -EFAULT;
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
       
       case device_read_e:
            err = copy_from_user(&my_msg, (struct msg __user *)data, sizeof(msg));
            printk(KERN_NOTICE "the input len is %d\n", my_msg.len);
            if(err)
            {
              return -EFAULT;
	    }
            if((64-r)>my_msg.len)
            {
              err = copy_to_user(my_msg.buff, g_keys+r, my_msg.len*4);
              printk(KERN_NOTICE "the output data is %x\n", my_msg.buff[0]);
              printk(KERN_NOTICE "the output data is %x\n", g_keys[r]);
              err = copy_to_user((struct msg __user *)data, &my_msg, sizeof(struct msg));
              if(err)
              {
                return -EFAULT;
              }
            }
             else 
            {
	       err = copy_to_user(my_msg.buff, g_keys+r, my_msg.len);
               err = copy_to_user((void *)data, &my_msg, sizeof(msg));
               if(err)
               {
                 return -EFAULT;
               }
               printk(KERN_NOTICE "the output data is %d\n", my_msg.buff[0]);
	    }
            r = (r + my_msg.len)%BUF_LEN;
            printk(KERN_NOTICE "r = %d\n", r);
            break;       

       case device_write_e:
	    err = copy_from_user(&my_msg, (struct msg __user *)data, sizeof(struct msg));
            
            printk(KERN_NOTICE "the input len is %d\n", my_msg.len);
            printk(KERN_NOTICE "the input data is %d\n", my_msg.buff[0]);
            printk(KERN_NOTICE "the input data2 is %d\n", my_msg.buff[1]);

            if(my_msg.len>(BUF_LEN-w1))
            {
               err = copy_from_user(g1_keys+w1, my_msg.buff, BUF_LEN-w1);
               err = copy_from_user(g1_keys, my_msg.buff+(BUF_LEN-w1), my_msg.len-(BUF_LEN-w1));
               if(err)
               {
                 return -EFAULT;
               }
            }else
            {
		err = copy_from_user(g1_keys+w1, my_msg.buff, my_msg.len*4);
                if(err)
                {
                   printk(KERN_NOTICE "error\n");
                }
            }
            w1 = (w1 + my_msg.len)%64;
            printk(KERN_NOTICE "w1 = %d\n", w1);
            printk(KERN_NOTICE "w1 = %d\n", g1_keys[0]);
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
    init_timer(&mytimer);
    mytimer.data = 250;
    mytimer.function = mytimer_task;
    mytimer.expires = jiffies + 1000;
    add_timer(&mytimer);
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
