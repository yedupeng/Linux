cmd_/home/cry/work/ioctl_work/modules.order := {   echo /home/cry/work/ioctl_work/hellomodule.ko; :; } | awk '!x[$$0]++' - > /home/cry/work/ioctl_work/modules.order
