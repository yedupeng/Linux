#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/nsc_gpio.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <asm/tc3162/tc3162.h>
#include <asm/tc3162/ledcetrl.h>
// #include "ioctl.h"

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static DEFINE_MUTEX(mymutex);

#define BUF_LEN 64
#define _ra_inl(addr) (*(volatile unsigned int *)(addr))
#define NEXT_POS(x) ((x+1)%BUF_LEN)
#define sleep_time 1
#define WLAN_LED_ON  1
#define WLAN_LED_OFF 0
#define WLAN_IIC_OEN  2
#define WLAN_IIC_IEN  3

#define address_pca9535 78
#define register7 7  
#define register3 3  

#define magic_id 'k'

static inline unsigned int regRead32(unsigned long reg)		\
{						  	\
	return readl(reg);			  	\
}

static inline void regWrite32(unsigned long reg, uint32 vlaue)	\
{                                                	\
	writel(vlaue, reg);                      	\
}

#define LED_GET_GPIO_DATA(x) ((x>31)?(regRead32(CR_GPIO_DATA1)&(1<<(x-32))):(regRead32(CR_GPIO_DATA)&(1<<x)))

int io_major = 201;
int io_minor = 0;
int numDevice = 1;
struct cdev cdev;
dev_t devno;
struct class *myClass;
struct timer_list mytimer;
static unsigned int *lenth;
static unsigned int value;
static int irqno;
unsigned int flag = 2;
unsigned int flag1 = 0;

#define LED_MODE(x)		((x) & LED_MODE_MASK) 
#define LED_BICOLOR(x)	((x) & LED_MODE_BICOLOR) 
#define LED_GPIO_MODE(x) (((x) & 0xff00) >> 8)
#define LED_GPIO1(x)	(((x) & 0xf0) >> 4)
#define LED_GPIO2(x)	((x) & 0x0f)
#define LED_GPIO1_NEW(x)	(((x) & 0xff00) >> 8)
#define LED_GPIO2_NEW(x)	((x) & 0xff)

#define REG_SETBITS(reg, bitsmask, bits) (regWrite32(reg, (regRead32(reg) & ~(bitsmask)) | (bits)))
#define REG_CLRBITS(reg, bits) (regWrite32(reg, regRead32(reg) & ~(bits)))

#define DO_IIC_SH(x) do {	gpio_spin_lock(); 					\
									if(x > 31){					\
										regWrite32(CR_GPIO_DATA1,regRead32(CR_GPIO_DATA1)|(1<<(x-32)));	\
									}else{						\
										regWrite32(CR_GPIO_DATA,regRead32(CR_GPIO_DATA)|(1<<x));	\
									}						\
							gpio_spin_unlock(); \
						} while (0)

#define DO_IIC_SL(x) do {	gpio_spin_lock();							\
									if(x > 31){					\
										regWrite32(CR_GPIO_DATA1,regRead32(CR_GPIO_DATA1)& ~(1<<(x-32)));	\
									}else{						\
										regWrite32(CR_GPIO_DATA,regRead32(CR_GPIO_DATA)& ~(1<<x));	\
									}						\
							gpio_spin_unlock(); \
						} while (0)

#define IIC_OEN(x)		do { 		\
									if(x > 31){							\
										if(x > 47){						\
											REG_SETBITS(CR_GPIO_CTRL3, 0x3<<((x-48)*2), 1<<((x-48)*2)); \
										}else{							\
											REG_SETBITS(CR_GPIO_CTRL2, 0x3<<((x-32)*2), 1<<((x-32)*2)); \
										}								\
										REG_SETBITS(CR_GPIO_ODRAIN1, 0x1<<(x-32), 1<<(x-32)); \
									}									\
									else{								\
										if(x > 15){						\
											REG_SETBITS(CR_GPIO_CTRL1,0x3<<((x-16)*2), 1<<((x-16)*2)); \
										}else{							\
											REG_SETBITS(CR_GPIO_CTRL,0x3<<((x)*2), 1<<((x)*2)); \
										}								\
										REG_SETBITS(CR_GPIO_ODRAIN, 0x1<<(x), 1<<(x)); \
									}									\
								\
				} while(0)
/* input enable */
#define IIC_IEN(x)		do { 		\
									if(x > 31){							\
										if(x > 47)						\
											REG_CLRBITS(CR_GPIO_CTRL3, (0x00000003 << ((x-48)* 2)));	\
										else							\
											REG_CLRBITS(CR_GPIO_CTRL2, (0x00000003 << ((x-32)* 2))); \
										REG_CLRBITS(CR_GPIO_ODRAIN1, (0x00000001 << (x-32))); \
									}									\
									else{								\
										if(x > 15)						\
											REG_CLRBITS(CR_GPIO_CTRL1,(0x00000003 << ((x-16)* 2)));	\
										else							\
											REG_CLRBITS(CR_GPIO_CTRL,(0x00000003 << (x* 2)));	\
										REG_CLRBITS(CR_GPIO_ODRAIN,(0x00000001 << (x))); \
									}									\
								\
				} while(0)

#define ra_inl(addr) (*(volatile unsigned int *)(addr))

static int gpio_init(void)
{
    // 输出使能
    IIC_OEN(1);
    IIC_OEN(27);
    // 写入数据
    DO_IIC_SH(27);
    DO_IIC_SH(1);
    IIC_IEN(26);
    return 0;
}

static int start(void)
{
    msleep(sleep_time);
    DO_IIC_SL(27);
    msleep(sleep_time);
    DO_IIC_SL(1);
    return 0;
}

static int stop(void)
{
    msleep(sleep_time);
    DO_IIC_SL(27);
    msleep(sleep_time);
    DO_IIC_SH(1);
    msleep(sleep_time);
    DO_IIC_SH(27);
    msleep(sleep_time);
    return 0;
}

static unsigned int rc_ack(void)
{
    unsigned int ack = 0;
    msleep(sleep_time);
    msleep(sleep_time);
    DO_IIC_SL(1);
    msleep(sleep_time);
    IIC_IEN(27);
    msleep(sleep_time);
    ack = LED_GET_GPIO_DATA(27);
    IIC_OEN(27);
    DO_IIC_SH(1);
    return ack;
}


static void sd_ack(void)
{
    DO_IIC_SL(27);
    msleep(sleep_time);
    DO_IIC_SH(1);
    msleep(sleep_time);
    msleep(sleep_time);

    DO_IIC_SL(1);
    return;
}

static void sd_nack(void)
{
    DO_IIC_SH(27);
    msleep(sleep_time);
    DO_IIC_SH(1);
    msleep(sleep_time);
    msleep(sleep_time);

    DO_IIC_SL(1);
    return;
} 

static unsigned int ii2_read(unsigned int *outData,int num)
{
    // 7
    unsigned int read_data = 0;
    unsigned int temp = 0;
    msleep(sleep_time);
    IIC_IEN(27);
    msleep(sleep_time);
    DO_IIC_SH(1);
    temp = LED_GET_GPIO_DATA(27)>>27;
    printk(KERN_INFO "temp = %d\n", temp);
    read_data = (temp&0x01)<<7;
    msleep(sleep_time);
    msleep(sleep_time);

    int i = 6;
    for(i;i>=0;i--)
    {
        DO_IIC_SL(1);
        msleep(sleep_time);
        msleep(sleep_time);
        temp = LED_GET_GPIO_DATA(27)>>27;
        printk(KERN_INFO "temp = %d\n", temp);
        read_data = ((temp&0x01)<<i)|read_data;
        DO_IIC_SH(1);
        msleep(sleep_time);
        msleep(sleep_time);
    }
    DO_IIC_SL(1);
    msleep(sleep_time);
    *outData = read_data;
    printk(KERN_INFO "read_data = %d\n", read_data);
    if(read_data)
    {
        sd_nack();
        printk(KERN_INFO "no ack\n");
    }
    else
    {
        sd_ack();
        printk(KERN_INFO "ack\n");
    }
    return 0;
}

static unsigned int write_data(unsigned int indata, int i)
{
    msleep(sleep_time);
    unsigned int out;
    out = (indata>>i) & 0x01;
    // printk(KERN_INFO "out = %d\n", out);
    if(out)
    {
        DO_IIC_SH(27);
    }
    else
    {
        DO_IIC_SL(27);
    }
    msleep(sleep_time);
    DO_IIC_SH(1);
}

static unsigned int iic_write(unsigned int indata)
{
    unsigned data_bit, ackSign;
    write_data(indata, 7);
    int i = 6;

    for(i; i>=0 ;i--)
    {
        msleep(sleep_time);
        msleep(sleep_time);
        DO_IIC_SL(1);
        write_data(indata, i);
    }
    ackSign = rc_ack();
    msleep(sleep_time);
    msleep(sleep_time);
    DO_IIC_SL(1);
    // printk(KERN_INFO "ack data is %d\n",ackSign>>26);
    // printk("\n");
    return ackSign;
}

void light_open(void)
{
    start();
    // address 0100 1110
    iic_write(address_pca9535);
    // contral 0000 0111
    iic_write(7);
    // data    0000 1101
    iic_write(13);
    stop();

    msleep(sleep_time);
    msleep(sleep_time);  
}

void light_close(void)
{
    msleep(sleep_time);
    msleep(sleep_time);
    start();
    // address  0100 1110
    iic_write(address_pca9535);
    // contral  0000 0011
    iic_write(3);
    // data     1111 1111
    iic_write(255);
    stop();
    msleep(sleep_time);  
}

void mytimer_task(struct timer_list  *timer)
{
    mod_timer(&mytimer,jiffies + msecs_to_jiffies(100));
    wake_up(&wait_queue);
    value = LED_GET_GPIO_DATA(26);
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

#define lightOpen      _IOWR('k', 0, int)
#define lightClose     _IOWR('k', 1, int)
#define lightShansuo    _IOWR('k', 3, int)

long int ioctl(struct file *node, unsigned int cmd, unsigned long data)
{
    long err;
    unsigned int max = 0;
    unsigned int indata = 1008611;

    unsigned int ret = 0;
    int i = 0;
    void *args = (void *)data;
    unsigned int *p = args; 
    
    switch(cmd)
    {
       case lightOpen:
            // 点灯
            start();
            // address 0100 1110
            iic_write(address_pca9535);
            // contral 0000 0111
            iic_write(7);
            // data    0000 1101
            iic_write(13);
            stop();

            msleep(sleep_time);
            msleep(sleep_time);
            break;
	    
       case lightClose:
            // 灭灯
            msleep(sleep_time);
            msleep(sleep_time);
            start();
            // address  0100 1110
            iic_write(address_pca9535);
            // contral  0000 0011
            iic_write(3);
            // data     1111 1111
            iic_write(255);
            stop();
            msleep(sleep_time);
            break;
        
        case lightShansuo:
            for(i; i < *p; i++)
            {
                light_open();
                msleep(100);
                light_close();
            }
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
    struct device *devices;
    devices = device_create(myClass, NULL, devno, NULL, "io_node");
    if(devices == NULL)
    {
        printk(KERN_NOTICE "device_create error\n");
    }
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
    timer_setup(&mytimer, mytimer_task, 0);
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
