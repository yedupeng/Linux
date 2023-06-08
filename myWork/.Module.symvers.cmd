cmd_/home/cry/myWork/Module.symvers := sed 's/\.ko$$/\.o/' /home/cry/myWork/modules.order | scripts/mod/modpost -m -a -E -o /home/cry/myWork/Module.symvers -e -i Module.symvers   -T -
