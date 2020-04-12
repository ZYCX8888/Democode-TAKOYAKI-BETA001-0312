mkdir /var/tmp
date 032912122020.12
ifconfig lo 127.0.0.1 up
ifconfig eth0 172.19.24.186 netmask 255.255.255.0
ifconfig eth0 hw ether 00:70:27:00:00:01
route add default gw 172.19.24.254
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/customer/lib:/customer/libdns:/lib
echo 12 >/sys/class/gpio/export
echo out >/sys/class/gpio/gpio12/direction
echo 1 >/sys/class/gpio/gpio12/value
cd /customer
chmod 777 zkgui
./zkgui &