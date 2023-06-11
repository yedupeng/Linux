#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include "ioctl.h"

MODULE_LICENSE ("GPL");
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

int major = 230;
int minor = 0;
int numDevice = 1;
char receive[128] = "Congratulations on getting the data";
char send[128] = {};
static int send_num = 666;
static int read_num = 777;
struct timer_list mytimer;
static int mask = 0;

void mytimer_task(struct timer_list  *timer)
{
    printk(KERN_INFO "this is time interrupt\n");
    if(mask < 999)
    {
        mask++;
    }else
    {
        mask = 0;
    }
    mod_timer(timer, jiffies + 100);
    wake_up_interruptible(&wait_queue);
}

struct cdev cdev;
struct class *myClass;
dev_t devno;

static int open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "finish open\n");
    return 0;
}

static int release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "finish release\n");
    return 0;
}

ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    ssize_t result;
    // 因为限制了接收数组和发送数组大小，为防止越界进行下述操作
    if(count>=128)
    {
        count = 127;
    }
    result = copy_to_user(buff, receive, count);
    if(result)
    {
        printk(KERN_NOTICE  "copy_to_user error\n");
        result = EFAULT;
    }else
    {
        result = count;
    }
    return result;
}

ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    ssize_t ret;
    if(count>=128)
    {
        count = 127;
    }
    ret = copy_from_user(send, buff, count);
    if(ret)
    {
        ret = EFAULT;
    }else
    {
        ret = count;
        send[count] = '\0';
        printk (KERN_INFO"Received: %s\n", send);
    }
    return ret;
}

long int ioctl(struct file *node, unsigned int cmd, unsigned long data)
{
    long err;
    void *args = (void *)data;
    int *p = args;

    // 判断幻数
    if(_IOC_TYPE(cmd) != IO_MAGIC) return -ENOTTY;

    // 判断序号是否超过最大值
    if(_IOC_NR(cmd) > device_max) return -ENOTTY;

     // 判断方向
    if (_IOC_DIR(cmd) & _IOC_READ || _IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok((void __user *)data, _IOC_SIZE(cmd));
        
    if (err)
        return -EFAULT;

    switch(cmd)
    {
    case device_read:
        err = get_user(read_num, p);
        printk(KERN_INFO "receive data:%d\n",read_num);
        break;
    
    case device_write:
        err = put_user(send_num, p);
        printk(KERN_INFO "send data:%d\n",send_num);
        break;

    default:
        printk(KERN_INFO "this is ioctl cmd\n");
        break;
    }
    return err;
}

unsigned int mem_poll(struct file *flip, poll_table *wait)
{
    unsigned int flag = 0;
    poll_wait(flip, &wait_queue, wait);
    printk(KERN_INFO "the mask is %d\n", mask);
    if (mask)         
        flag = POLLIN | POLLRDNORM; 
    return flag;
}

struct file_operations hello_fops = 
{
    .owner = THIS_MODULE,
    .open = open,
    .poll = mem_poll,
    .release = release,
    .read = read,
    .write = write,
    .unlocked_ioctl = ioctl
};

static int cdev_create(void)
{
    // 使用device_create首先要创建类
    myClass = class_create(THIS_MODULE, "hello");
    if(IS_ERR(myClass))
    {
        printk(KERN_NOTICE  "class_create error\n");
        return -1;
    }
    // 创建设备结点
    device_create(myClass, NULL, devno, NULL, "hello_node");
    return 0;
}

static void setup_cdev(void)
{
    int err;
    // 用于初始化cdev结构体，并填充其成员ops
    cdev_init(&cdev, &hello_fops);
    // 指明拥有者
    cdev.owner = THIS_MODULE;
    // 注册设备
    err = cdev_add(&cdev, devno, numDevice);
    if(err)
        printk(KERN_NOTICE  "cdev_add error\n");
}

static int __init myModule1(void)
{
    int result;
    devno = MKDEV(major, minor);

    mytimer.expires = jiffies + 100;
    timer_setup(&mytimer, mytimer_task, 0);
    add_timer(&mytimer);
    // 设备号注册，用于后续字符设备注册
    result = register_chrdev_region(devno, numDevice, "hello");
    if(result<0)
    {
        printk (KERN_WARNING "hello: can't get major number %d\n", major);
        return result;
    }
    cdev_create();
    setup_cdev();
    printk (KERN_INFO "init finish\n");
    return 0;
}

static void __exit myModule2(void)
{
    cdev_del(&cdev);
    device_destroy(myClass, devno);
    class_destroy(myClass);
    del_timer(&mytimer);
    unregister_chrdev_region(devno, numDevice);
    printk (KERN_INFO "exit finish\n");
}

module_init(myModule1);
module_exit(myModule2);