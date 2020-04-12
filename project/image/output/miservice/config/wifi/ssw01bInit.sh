#!/bin/sh

/config/riu_w e 30 11
/config/riu_w 103c 8 00
sleep 0.01 
/config/riu_w 103c 8 10
#mkdir -p /etc/
#touch /etc/hosts
touch /appconfigs/hosts
mkdir -p /tmp/wifi/run
chmod 777 /tmp/wifi/run
mkdir -p /appconfigs/misc/wifi/
mkdir -p /var/wifi/misc/
mkdir -p /var/lib/misc/
mkdir -p /var/run/hostapd/
insmod /config/wifi/8188gtvu.ko
sleep 3






