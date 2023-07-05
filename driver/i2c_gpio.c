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
#include "i2c.h"

void delay(unsigned int time_num)
{
    while(time_num--);
}

static inline unsigned int regRead32(unsigned long reg)		\
{						  	\
	return readl(reg);			  	\
}

static inline void regWrite32(unsigned long reg, uint32 vlaue)	\
{                                                	\
	writel(vlaue, reg);                      	\
}

#define LED_GET_GPIO_DATA(x) ((x>31)?(regRead32(CR_GPIO_DATA1)&(1<<(x-32))):(regRead32(CR_GPIO_DATA)&(1<<x)))

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
    IIC_OEN(SCL);
    IIC_OEN(SDA);
    // 写入数据
    DO_IIC_SH(SDA);
    DO_IIC_SH(SCL);
    IIC_IEN(INT);
    return 0;
}

static int start(void)
{
    delay(sleep_time);
    DO_IIC_SL(SDA);
    delay(sleep_time);
    DO_IIC_SL(SCL);
    return 0;
}

static int stop(void)
{
    delay(sleep_time);
    DO_IIC_SL(SDA);
    delay(sleep_time);
    DO_IIC_SH(SCL);
    delay(sleep_time);
    DO_IIC_SH(SDA);
    delay(sleep_time);
    return 0;
}

static unsigned int rc_ack(void)
{
    unsigned int ack = 0;
    delay(sleep_time);
    delay(sleep_time);
    DO_IIC_SL(SCL);
    delay(sleep_time);
    IIC_IEN(SDA);
    delay(sleep_time);
    ack = LED_GET_GPIO_DATA(SDA);
    IIC_OEN(SDA);
    DO_IIC_SH(SCL);
    return ack;
}

static void sd_ack(void)
{
    DO_IIC_SL(SDA);
    delay(sleep_time);
    DO_IIC_SH(SCL);
    delay(sleep_time);
    delay(sleep_time);

    DO_IIC_SL(SCL);
    return;
}

static void sd_nack(void)
{
    DO_IIC_SH(SDA);
    delay(sleep_time);
    DO_IIC_SH(SCL);
    delay(sleep_time);
    delay(sleep_time);

    DO_IIC_SL(SCL);
    return;
} 

static unsigned int ii2_read(int is_nack)
{
    // 7
    unsigned int read_data = 0;
    unsigned int temp = 0;
    IIC_IEN(SDA);

    int i = 7;
    for(i;i>=0;i--)
    {
        DO_IIC_SL(SCL);
        delay(sleep_time);
        delay(sleep_time);
        DO_IIC_SH(SCL);
        delay(sleep_time);
        temp = LED_GET_GPIO_DATA(SDA)>>27;
        read_data |= ((temp&0x01)<<i);
        delay(sleep_time);
    }
    DO_IIC_SL(SCL);
    delay(sleep_time);
    IIC_OEN(SDA);
    if(!read_data || is_nack)
    {
        sd_nack();
        printk(KERN_INFO "no ack\n");
    }
    else
    {
        sd_ack();
        printk(KERN_INFO "ack\n");
    }
    return read_data;
}

static unsigned int write_data(unsigned int indata, int i)
{
    delay(sleep_time);
    unsigned int out;
    out = (indata>>i) & 0x01;
    // printk(KERN_INFO "out = %d\n", out);
    if(out)
    {
        DO_IIC_SH(SDA);
    }
    else
    {
        DO_IIC_SL(SDA);
    }
    delay(sleep_time);
    DO_IIC_SH(SCL);
}

static unsigned int iic_write(unsigned int indata)
{
    unsigned data_bit, ackSign;
    write_data(indata, 7);
    int i = 6;

    for(i; i>=0 ;i--)
    {
        delay(sleep_time);
        delay(sleep_time);
        DO_IIC_SL(SCL);
        write_data(indata, i);
    }
    ackSign = rc_ack();
    delay(sleep_time);
    delay(sleep_time);
    DO_IIC_SL(SCL);
    // printk(KERN_INFO "ack data is %d\n",ackSign>>INT);
    // printk("\n");
    return ackSign;
}

void light_open(void)
{
    start();
    iic_write(write_address);
    // contral 0000 0111
    iic_write(Config_Port_1);
    // data    0000 1101
    iic_write(13);
    stop();

    delay(sleep_time);
    delay(sleep_time);

    start();
    // address 0100 1110
    iic_write(write_address);
    // contral 0000 0011
    iic_write(Out_Port1);
    // data    1111 1101
    iic_write(253);
    stop();
    delay(sleep_time);
}

void light_close(void)
{
    delay(sleep_time);
    delay(sleep_time);
    start();
    // address  0sleep_time 1110
    iic_write(write_address);
    // contral  0000 0011
    iic_write(Out_Port1);
    // data     1111 1111
    iic_write(255);
    stop();
    delay(sleep_time);
}

void mytimer_task(struct timer_list  *timer)
{
    unsigned int temp,temp1 = 0; 
    mod_timer(&mytimer,jiffies + msecs_to_jiffies(4000));
    wake_up(&wait_queue);
    value = LED_GET_GPIO_DATA(INT);
    start();
    iic_write(write_address);
    iic_write(In_Port0);
    delay(sleep_time);
    delay(sleep_time); 
    DO_IIC_SH(SCL);
    DO_IIC_SH(SDA);
    start();
    iic_write(read_address);
    temp = ii2_read(0);
    temp1 = ii2_read(1);
    stop();
    printk(KERN_INFO "the value2 is %d\n", value>>26);
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

    unsigned int ret = 0;
    int i = 0;
    void *args = (void *)data;
    unsigned int *p = args; 
    unsigned int temp,temp1 = 0; 
    switch(cmd)
    {
       case lightOpen:
            light_open();
            break;
	    
       case lightClose:
            light_close();
            break;
        
        case lightShansuo:
            for(i; i < 100 ;i++)
            {
                start();
                iic_write(write_address);
                iic_write(Config_Port_1);
                iic_write(13);
                stop();

                delay(sleep_time);
                delay(sleep_time);

                start();
                iic_write(write_address);
                iic_write(Out_Port1);
                if(i % 2 == 0)
                {
                    iic_write(253);
                }else
                {
                    iic_write(255);
                }
                stop();
                msleep(50);
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

static int key_init(void)
{
    IIC_IEN(INT);
    start();
    iic_write(write_address);
    iic_write(6);
    iic_write(255);
    stop();   

    value = LED_GET_GPIO_DATA(INT);
    printk(KERN_INFO "the key's value is %d\n", value>>INT);
    return 0;
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
    mytimer.expires = jiffies + HZ;
    timer_setup(&mytimer, mytimer_task, 0);
    add_timer(&mytimer);
    gpio_init();
    key_init();
    printk(KERN_NOTICE "init\n");

    //dev_set();
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