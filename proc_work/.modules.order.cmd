cmd_/home/cry/work/proc_work/modules.order := {   echo /home/cry/work/proc_work/proc.ko; :; } | awk '!x[$$0]++' - > /home/cry/work/proc_work/modules.order
