#ifndef __IOCTL__H
#define __IOCTL__H

#define magic_id 'k'

#define lightOpen _IOWR(magic_id, 0, int)
#define lightClose _IOWR(magic_id, 1, int)
#define lightShansuo _IOWR(magic_id, 3, int)


#define device_max 3


#endif
