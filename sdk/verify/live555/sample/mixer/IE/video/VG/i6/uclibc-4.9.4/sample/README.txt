1. Creating executable files on Linux PC:
    a. Sample code is Test_VG_Code.c
    a. Lib is libVG_Linux_I1.a/libVG_Linux_I1.so in ../lib/
    b. Type "make run_VG_I1" in your Linux desktop environment
    c. Makefile will generate two executable files in ../bin/:
    run_VG_I1_linux_dynamic and run_VG_I1_linux_static. They are built using
    .so and .a, respectively

2. Executable files Usage:
    $ ./run_VG_I1_linux_dynamic <filename>
    $ ./run_VG_I1_linux_static <filename>
