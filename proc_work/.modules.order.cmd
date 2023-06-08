cmd_/home/cry/Work/proc_work/modules.order := {   echo /home/cry/Work/proc_work/proc.ko; :; } | awk '!x[$$0]++' - > /home/cry/Work/proc_work/modules.order
