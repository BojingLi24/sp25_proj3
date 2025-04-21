Here are hints to help you set up the system calls:
1.You need to change the linux Makefile, line 1152
2.Use the provided project Makefile, you do not need to change it
2.You need to change the syscall_64.tbl according to the screenshot in this directory
3.You need to change the syscall.h according to the screenshot in this directory



compile and run the program:
gcc -o proj3 main.c -lpthread
./proj3
