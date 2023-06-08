#ifndef __IOCTL__h
#define __IOCTL__H

#define IO_MAGIC 'k'

#define device_read _IOR(IO_MAGIC, 0, int)
#define device_write _IOW(IO_MAGIC, 1, int)

#define device_max 2

#endif