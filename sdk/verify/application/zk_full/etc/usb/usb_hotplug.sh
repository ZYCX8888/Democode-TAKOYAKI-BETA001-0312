#!/bin/sh
if [ $ACTION = "add" ]; then 
	mkdir -p /vendor/udisk_$DEVNAME
	mount -o iocharset=utf8 /dev/$DEVNAME /vendor/udisk_$DEVNAME
	if [ $? -ne 0 ];then 
	    rm -rf /vendor/udisk_$DEVNAME
	fi
else 
        umount /vendor/udisk_$DEVNAME
        if [ $? -eq 0 ];then
            rm -rf /vendor/udisk_$DEVNAME
        fi
fi
