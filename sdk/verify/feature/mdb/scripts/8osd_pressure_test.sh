#!/bin/sh
export PATH=${PATH}:/config/mdb/
./service &
osd_pressure() {
	./mdbcmd region
	./mdbcmd init
	while true
	do
		./mdbcmd push 1
		./mdbcmd 1 injectword 44 0 0 1 300 48 320 16 100 $(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
		./mdbcmd 1 injectword 55 0 0 1 300 64 320 16 100 $(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
		./mdbcmd 1 injectword 66 0 0 1 300 80 320 16 100 $(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
		./mdbcmd 1 injectword 77 0 0 1 300 96 320 16 100 $(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
		./mdbcmd 1 injectword 88 0 0 1 300 112 320 16 100 $(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
		./mdbcmd 1 injectword 99 0 0 1 300 128 320 16 100 $(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
		./mdbcmd 1 attachosd 44 1 0 2 0 1 300 48 100 0 0 0 0 0 0 0 0xFF
		./mdbcmd 1 attachosd 55 1 0 2 0 1 300 64 100 0 0 0 0 0 0 0 0xFF
		./mdbcmd 1 attachosd 66 1 0 2 0 1 300 80 100 0 0 0 0 0 0 0 0xFF
		./mdbcmd 1 attachosd 77 1 0 2 0 1 300 96 100 0 0 0 0 0 0 0 0xFF
		./mdbcmd 1 attachosd 88 1 0 2 0 1 300 112 100 0 0 0 0 0 0 0 0xFF
		./mdbcmd 1 attachosd 99 1 0 2 0 1 300 128 100 0 0 0 0 0 0 0 0xFF
		./mdbcmd 1 create 0 0 0 200 356
		./mdbcmd 1 create 1 0 0 300 224
		./mdbcmd 1 attachosd 0 0 0 0 1 1 133 200 99 0 0 0 0 0 0 0 255
		./mdbcmd 1 attachosd 0 1 0 2 0 1 133 200 99 0 0 0 0 0 0 0 255
		./mdbcmd 1 attachosd 1 0 0 0 1 1 277 365 101 0 0 0 0 0 0 0 255
		./mdbcmd 1 attachosd 1 1 0 2 0 1 277 365 101 0 0 0 0 0 0 0 255
		./mdbcmd 1 setbitmap 1 6 0
		./mdbcmd 1 setcanvas 0 4 0
		./mdbcmd pop 1
		sleep 1
		./mdbcmd push 1
		./mdbcmd 1 setramdompos 0 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 1 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 44 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 55 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 66 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 77 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 88 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 99 0 0 1 0 1920 0 1080
		./mdbcmd 1 setramdompos 0 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 1 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 44 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 55 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 66 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 77 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 88 1 2 0 0 720 0 576
		./mdbcmd 1 setramdompos 99 1 2 0 0 720 0 576
		./mdbcmd pop 1
		idx=0
		while [ $idx -lt 20 ]
		do
			./mdbcmd push 1
			./mdbcmd 1 updateword 44 handle44_$(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
			./mdbcmd 1 updateword 55 handle55_$(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
			./mdbcmd 1 updateword 66 handle66_$(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
			./mdbcmd 1 updateword 77 handle77_$(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
			./mdbcmd 1 updateword 88 handle88_$(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
			./mdbcmd 1 updateword 99 handle99_$(date "+%Y/%m/%d-%H:%M:%S/")$RANDOM
			./mdbcmd pop 1
			let idx=$idx+1
			usleep 33000
		done
		./mdbcmd push 1
		./mdbcmd 1 elicitword 44
		./mdbcmd 1 elicitword 55
		./mdbcmd 1 elicitword 66
		./mdbcmd 1 elicitword 77
		./mdbcmd 1 elicitword 88
		./mdbcmd 1 elicitword 99
		./mdbcmd 1 destroy 1
		./mdbcmd 1 dettach 0 0 0 0 1
		./mdbcmd 1 dettach 0 1 0 2 0
		./mdbcmd 1 destroy 0
		./mdbcmd pop 1
		sleep 5
	done
	./mdbcmd q
}
./mdbcmd w
./mdbcmd integrate
./mdbcmd vifinit 1920 1080 0
./mdbcmd vpeinit 0 1920 1080 0x18
./mdbcmd iqopenserver 1080 1080 0
./mdbcmd setbind2 6 0 0 0 30 7 0 0 0 30 0x4 0
./mdbcmd q
./mdbcmd integrate
./mdbcmd vpecreateport 0 1 1920 1080 11
./mdbcmd vencinit 1920 1080 1 2
#./mdbcmd vencsetsrc 1 2
./mdbcmd vencstart 1
#./mdbcmd setbind2 7 0 0 1 30 2 0 1 0 30 0x10 540
./mdbcmd rtspstart stream1 2
./mdbcmd q
./mdbcmd integrate
./mdbcmd vpecreateport 0 2 720 576 11
./mdbcmd divpinit 2 0 0 0 0 0 0 0
./mdbcmd divpcreateport 2 11 720 576
./mdbcmd setmma 12 0 2 NULL
./mdbcmd divpstart 2
./mdbcmd setbind2 7 0 0 2 30 12 0 2 0 30 1 0
./mdbcmd vencinit 720 576 2 2
./mdbcmd setmma 2 0 2 NULL
./mdbcmd vencstart 2
./mdbcmd setbind 12 0 2 0 30 2 0 2 0 30
./mdbcmd rtspstart stream2 2
./mdbcmd q
osd_pressure