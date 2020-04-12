1. Creating executable files on Linux PC:
    a. Sample code is sample.c
    a. Lib is libLSD_LinuxC3.a/libLSD_LinuxC3.so in ../lib/
    b. Type "make run_LSD_C3" in your Linux desktop environment
    c. Makefile will generate two executable files in ../bin/:
    run_LSD_C3_linux_dynamic and run_LSD_C3_linux_static. They are built using
    .so and .a, respectively

2. Executable files Usage:
    $ ./run_LSD_C3_linux_dynamic <filename>
    $ ./run_LSD_C3_linux_static <filename>
