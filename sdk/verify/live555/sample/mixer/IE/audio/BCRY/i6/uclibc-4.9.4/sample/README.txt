1. Creating executable files on Linux PC:
    a. Sample code is sample.c
    a. Lib is libAED_LINUX.a/libAED_LINUX.so in ../lib/
    b. Type "make run_AED_I3" in your Linux desktop environment
    c. Makefile will generate two executable files in ../bin/:
    run_AED_I3_linux_dynamic and run_AED_I3_linux_static. They are built using
    .so and .a, respectively

2. Executable files Usage:
    $ ./run_AED_I3_linux_dynamic <filename>
    $ ./run_AED_I3_linux_static <filename>
