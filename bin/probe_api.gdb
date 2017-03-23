# gdb -batch -x <gdb script name> <executable name> <pid>
b nt_listener.c:46
continue
print *header
#printf "gwid:%llx, chid:%d, uid:%lld\n", gwid, chid, uid
#set scheduler-locking on                             # 如果要在gdb中执行函数(如下一行), 务必加上这一行以确保在函数执行时不会因为其他线程被信号中断导致进程退出.
#p inspector::valid_session_num(*(svc_.px))