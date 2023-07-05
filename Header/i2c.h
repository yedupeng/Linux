#ifndef __IIC__H
#define __IIC__H

/******************************   chip port    ******************************/
#define SCL 1
#define SDA 27
#define INT 26


/***************************** PCA9535 register *****************************/
// address
#define write_address 0x4E
#define read_address 0x4F

// command 111~0
#define Config_Port_1 7  
#define Config_Port_0 6 
#define Pol_Port1 5
#define Pol_Port0 4
#define Out_Port1 3
#define Out_Port0 2 
#define In_Port1 1
#define In_Port0 0

/********************************** data *************************************/
#define magic_id 'k'

#define led_name "gpio_led"
static DEFINE_SPINLOCK(test_lock);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static DEFINE_MUTEX(mymutex);

#define BUF_LEN 64
#define _ra_inl(addr) (*(volatile unsigned int *)(addr))
#define NEXT_POS(x) ((x+1)%BUF_LEN)
#define sleep_time 100

int io_major = 201;
int io_minor = 0;
int numDevice = 1;

struct cdev cdev;
dev_t devno;
struct class *myClass;
struct timer_list mytimer;
static unsigned int value;

#endif