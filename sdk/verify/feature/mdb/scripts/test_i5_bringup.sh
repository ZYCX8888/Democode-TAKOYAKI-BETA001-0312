#!/bin/sh

port_array="0 1 2 3"
port0_x=1800
port0_y=980
port1_x=1160
port1_y=620
port2_x=1160
port2_y=620
port3_x=1800
port3_y=980
port_x="0"
port_y="0"

do_choice_port() {
	if [ "$1" == "0" ]; then
		port_x="$port0_x"
		port_y="$port0_y"
		echo "port 0"
	fi
	if [ "$1" == "1" ]; then
		port_x="$port1_x"
		port_y="$port1_y"
		echo "port 1"
	fi
	if [ "$1" == "2" ]; then
		port_x="$port2_x"
		port_y="$port2_y"
		echo "port 1"
	fi
	if [ "$1" == "3" ]; then
		port_x="$port3_x"
		port_y="$port3_y"
		echo "port 3"
	fi
}
do_region_cmd() {
	./mdbenter region
	./mdbcmd injectpic $2 0 $3 $4 1 $1 $14 $15 $16 $5 $6 0
	./mdbexit
	read -p "next testcase >"

	./mdbenter region
	./mdbcmd injectpic $7 1 $8 $9 1 $1 $14 $15 $16 $10 $11 0
	./mdbexit
	read -p "next testcase >"

	./mdbenter region
	./mdbcmd setosddisplayattr 1 $14 0 $15 $16 1 $12 $13 0 0 0 0 0 0
	./mdbexit
	read -p "next testcase >"

	./mdbenter region
	./mdbcmd setosddisplayattr 1 $14 0 $15 $16 1 $port_x $port_y 0 0 0 0 0 0
	./mdbexit
	read -p "next testcase >"
	
	./mdbenter region
	./mdbcmd destroy 0
	./mdbcmd destroy 1
	./mdbexit
}
do_region_osd() {
	for port in $port_array
	do
		do_choice_port $port
		if [ "$port" == "2" ]; then
			do_region_cmd $1 $2 $3 $4 $5 $6 $7 $8 $9 $10 $11 $12 $13 1 $port 0
		else
			do_region_cmd $1 $2 $3 $4 $5 $6 $7 $8 $9 $10 $11 $12 $13 0 0 $port
		fi
	done
}
do_region_cover() {
	for port in $port_array
	do
		./mdbenter region
		./mdbcmd injectcover 0 2000 2000 0 0 $port 1000 1000 0 0xFF
		./mdbexit
		read -p "next testcase >"

		./mdbenter region
		./mdbcmd injectcover 1 2500 2500 0 0 $port 1500 1500 1 0xFF00
		./mdbcmd injectcover 2 3000 1000 0 0 $port 2000 2000 2 0xFF0000
		./mdbcmd injectcover 3 1000 2000 0 0 $port 2500 2500 3 0x0
		./mdbexit
		read -p "next testcase >"

		./mdbenter region
		./mdbcmd destroy 0
		./mdbcmd destroy 1
		./mdbcmd destroy 2
		./mdbcmd destroy 3
		./mdbexit
	done
}
./mdbenter region
./mdbcmd init
./mdbexit
do_region_osd 0 /mnt/1555/128X96.argb1555 128 96 0 0 /mnt/1555/128X96.argb1555 128 96 28 200 140 28
do_region_osd 1 /mnt/4444/128X96.argb4444 128 96 0 0 /mnt/4444/128X96.argb4444 128 96 28 200 140 28
do_region_osd 2 /mnt/I2/64X48.i2 64 48 0 0 /mnt/I2/200X200.i2 200 200 28 200 100 28
do_region_osd 3 /mnt/I4/64X48.i4 64 48 0 0 /mnt/I4/200X200.i4 200 200 28 200 100 28
do_region_osd 4 /mnt/I8/64X48.i8 64 48 0 0 /mnt/I8/200X200.i8 200 200 28 200 100 28
do_region_osd 5 /mnt/565/128X96.rgb565 128 96 0 0 /mnt/565/128X96.rgb565 128 96 28 200 140 28
do_region_osd 6 /mnt/8888/128X96.argb8888 128 96 0 0 /mnt/8888/128X96.argb8888 128 96 28 200 140 28
do_region_cover