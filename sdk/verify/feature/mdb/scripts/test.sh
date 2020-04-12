#!/bin/sh

port_array="0 1 2 3"

do_region_osd() {
	for port in $port_array
	do
		./mdbenter region
		./mdbcmd injectpic $2 0 $3 $4 1 $1 0 0 $port $5 $6
		./mdbexit
		./mdbenter integrate
		./mdbcmd vencwritefile /tmp/data.es $port 3
		./mdbexit

		./mdbenter region
		./mdbcmd injectpic $7 1 $8 $9 1 $1 0 0 $port $10 $11
		./mdbexit
		./mdbenter integrate
		./mdbcmd vencwritefile /tmp/data.es $port 3
		./mdbexit

		./mdbenter region
		./mdbcmd setdisplayattr 1 0 0 0 $port 1 $12 $13 0 0 0 0
		./mdbexit
		./mdbenter integrate
		./mdbcmd vencwritefile /tmp/data.es $port 3
		./mdbexit

		./mdbenter region
		./mdbcmd destroy 0
		./mdbcmd destroy 1
		./mdbexit
	done
}
do_region_cover() {
	for port in $port_array
	do
		./mdbenter region
		./mdbcmd injectcover 0 2000 2000 0 0 $port 1000 1000 0 0xFF
		./mdbexit
		./mdbenter integrate
		./mdbcmd vencwritefile /tmp/data.es $port 3
		./mdbexit

		./mdbenter region
		./mdbcmd injectcover 1 2500 2500 0 0 $port 1500 1500 1 0xFF00
		./mdbcmd injectcover 2 3000 1000 0 0 $port 2000 2000 2 0xFF0000
		./mdbcmd injectcover 3 1000 2000 0 0 $port 2500 2500 3 0x0
		./mdbexit
		./mdbenter integrate
		./mdbcmd vencwritefile /tmp/data.es $port 3
		./mdbexit

		./mdbenter region
		./mdbcmd destroy 0
		./mdbcmd destroy 1
		./mdbcmd destroy 2
		./mdbcmd destroy 3
		./mdbexit
	done
}
do_baseinit() {
	./mdbinit

	./mdbenter integrate
	./mdbcmd vpevifinit $1 $2
	./mdbcmd setbind 6 0 0 0 30 7 0 0 0 30
	for port in $port_array
	do
		./mdbcmd vpecreateport $1 $2 10 $port
		./mdbcmd vencinit $1 $2 $port 2
		./mdbcmd vencstart $port
		./mdbcmd setbind 7 0 0 $port 30 2 0 $port 0 30
	done
	./mdbexit

	./mdbenter region
	./mdbcmd init
	./mdbexit
}
do_basedeinit() {
	./mdbenter region
	./mdbcmd deinit
	./mdbexit

	./mdbenter integrate
	for port in $port_array
	do
		./mdbcmd setunbind  7 0 0 $port 12 0 $port 0
		./mdbcmd vencstop $port
		./mdbcmd vencdeinit $port
		./mdbcmd vpedestroyport $port
	done
	./mdbcmd setunbind 6 0 0 0 7 0 0 0
	./mdbcmd vpevifdeinit
	./mdbexit

	./mdbdeinit
}
do_exportfile() {
	./mdbenter integrate
	./mdbcmd exportfile /tmp/data.es /dev/mtd2
	./mdbexit
}

do_baseinit 352 288
do_region_osd 0 /data/ut/1555/128X96.argb1555 128 96 0 0 /data/ut/1555/128X96.argb1555 128 96 28 200 140 28
do_region_osd 1 /data/ut/4444/128X96.argb4444 128 96 0 0 /data/ut/4444/128X96.argb4444 128 96 28 200 140 28
do_region_osd 2 /data/ut/I2/64X48.i2 64 48 0 0 /data/ut/I2/200X200.i2 200 200 28 200 100 28
do_region_osd 3 /data/ut/I4/64X48.i4 64 48 0 0 /data/ut/I4/200X200.i4 200 200 28 200 100 28
do_region_osd 4 /data/ut/I8/64X48.i8 64 48 0 0 /data/ut/I8/200X200.i8 200 200 28 200 100 28
do_region_osd 5 /data/ut/565/128X96.argb565 128 96 0 0 /data/ut/565/128X96.argb565 128 96 28 200 140 28
do_region_osd 6 /data/ut/8888/128X96.argb8888 128 96 0 0 /data/ut/8888/128X96.argb8888 128 96 28 200 140 28
do_region_cover
do_exportfile
#do_basedeinit