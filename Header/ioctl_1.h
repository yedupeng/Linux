#ifndef __IOCTL__H
#define __IOCTL__H

#define magic_id 'k'

#define light_open _IOR(magic_id, 0, int)
#define light_close _IOW(magic_id, 1, int)

#define device_max 2


#endif
