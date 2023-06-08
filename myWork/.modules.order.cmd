cmd_/home/cry/myWork/modules.order := {   echo /home/cry/myWork/hellomodule.ko; :; } | awk '!x[$$0]++' - > /home/cry/myWork/modules.order
