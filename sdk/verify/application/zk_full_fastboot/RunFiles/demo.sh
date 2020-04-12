date 032912122020.12
mkdir /var/tmp
#echo 7M > /sys/block/zram0/disksize
#mkswap /dev/zram0
#swapon /dev/zram0
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/customer_app/lib:/lib:/config/lib:/customer/lib
echo 12 >/sys/class/gpio/export
echo out >/sys/class/gpio/gpio12/direction
echo 1 >/sys/class/gpio/gpio12/value
cd /customer_app
chmod 777 zkgui
./zkgui &
