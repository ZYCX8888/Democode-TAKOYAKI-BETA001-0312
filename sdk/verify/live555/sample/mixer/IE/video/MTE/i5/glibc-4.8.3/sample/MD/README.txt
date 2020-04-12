1. Creating executable files on Linux PC:
   a. Please type "make run_MD_I3" in your Linux desktop environment
   b. Sample code is mi_sample_md.c

2. Running executable files on ARM Linux:
   a. Lib is libMTE_Linux_I3.a/libMTE_Linux_I3.so (Note: this is for Linux Kernel 3.18)
   b. Executable file is run_MD_I3_linux_static/run_MD_I3_linux_dynamic which are created in step 1.a
   c. Library path for run_MD_I3_linux_dynamic can be changed by using -Wl,-rpath=/your/so/lib/path in Makefile


