#!/bin/sh
port_array="0 1 2 3"
disp_rgn="n"
port0_case=""
port1_case=""
port2_case=""
port3_case=""
src_fps=""
dst_fps=""
vifworkmode=""
vpeworkmode=""
vifwidth=""
vifheight=""
cmd=""

check_rec() {
	read -p "Do you want to record the commands.(yes/no)" select
	if [ "${select}" == "yes" -o "${select}" == "y" ]; then
		export MDB_REC_PATH=/tmp
		cmd="./mdbrec"
		echo "#!/bin/sh" > ${MDB_REC_PATH}/rec.sh
		echo "export PATH=\${PATH}:/config/mdb/" >> /tmp/rec.sh
		echo "./service &" >> ${MDB_REC_PATH}/rec.sh
	else
		cmd="./mdbcmd"
	fi
}

start_rtsp() {
	${cmd} integrate
	${cmd} vpecreateport 0 ${1} ${2} ${3} 11 
	${cmd} vencinit ${2} ${3} ${1} ${4}
	${cmd} vencstart ${1}
	${cmd} setbind 7 0 0 ${1} ${src_fps} 2 0 ${1} 0 ${dst_fps}
	${cmd} rtspstart stream${1} ${4}
	${cmd} q
}
start_rtsp_with_dip() {
	${cmd} integrate
	${cmd} vpecreateport 0 ${1} ${vifwidth} ${vifheight} 11
	${cmd} divpinit ${1} 0 0 0 0 0 0 0
	${cmd} divpcreateport ${1} 11 ${2} ${3}
	read -p "DIVP/VENC miu sel(0/1):" select
	if [ ${select} -eq 1 ]; then
		mma_name="mma_heap_name1"
	else
		mma_name="NULL"
	fi
	${cmd} setmma 12 0 ${1} ${mma_name}
	${cmd} divpstart ${1}
	${cmd} setbind 7 0 0 ${1} ${src_fps} 12 0 ${1} 0 ${dst_fps}
	${cmd} vencinit ${2} ${3} ${1} ${4}
	${cmd} setmma 2 0 ${1} ${mma_name}
	${cmd} vencstart ${1}
	${cmd} setbind 12 0 ${1} 0 ${src_fps} 2 0 ${1} 0 ${dst_fps}
	${cmd} rtspstart stream${1} ${4}
	${cmd} q
}
stop_rtsp() {
	${cmd} integrate
	${cmd} rtspstop stream${1}
	${cmd} setunbind  7 0 0 ${1} 2 0 ${1} 0
	${cmd} vencstop ${1}
	${cmd} vencdeinit ${1}
	${cmd} vpedestroyport ${1} 0
	${cmd} q
}
stop_rtsp_with_dip() {
	${cmd} integrate
	${cmd} rtspstop stream${1}
	${cmd} setunbind 12 0 ${1} 0 2 0 ${1} 0
	${cmd} vencstop ${1}
	${cmd} vencdeinit ${1}
	${cmd} divpstop ${1}
	${cmd} setunbind 7 0 0 ${1} 12 0 ${1} 0
	${cmd} divpdeinit ${1}
	${cmd} vpedestroyport ${1} 0
	${cmd} q
}
do_rtsp_capture_jpeg() {
	${cmd} integrate
	${cmd} vpecreateport 0 ${1} ${2} ${3} 11
	${cmd} vencinit ${2} ${3} ${1} 0
	${cmd} vencstart ${1}
	${cmd} setbind 7 0 0 ${1} ${src_fps} 2 0 ${1} 0 ${dst_fps}
	${cmd} vencwritefile /mnt/capture$(date "+%Y%m%d-%H%M%S").jpg ${1} 0 1
	${cmd} setunbind 7 0 0 ${1} 2 0 ${1} 0
	${cmd} vencstop ${1}
	${cmd} vencdeinit ${1}
	${cmd} vpedestroyport ${1} 0
	${cmd} q
}
do_choice_port_on() {
	if [ "${1}" == "0" ]; then
		port0_case="${2}"
	fi
	if [ "${1}" == "1" ]; then
		port1_case="${2}"
	fi
	if [ "${1}" == "2" ]; then
		port2_case="${2}"
	fi
	if [ "${1}" == "3" ]; then
		port3_case="${2}"
	fi
}
do_choice_port_off() {
	do_divp_flow=""
	if [ "${1}" == "0" ]; then
		do_divp_flow="${port0_case}"
		port0_case=""
	fi
	if [ "${1}" == "1" ]; then
		do_divp_flow="${port1_case}"
		port1_case=""
	fi
	if [ "${1}" == "2" ]; then
		do_divp_flow="${port2_case}"
		port2_case=""
	fi
	if [ "${1}" == "3" ]; then
		do_divp_flow="${port3_case}"
		port3_case=""
	fi
	if [ "$do_divp_flow" == "0" ]; then
		stop_rtsp ${1}
	fi
	if [ "$do_divp_flow" == "1" ]; then
		stop_rtsp_with_dip ${1}
	fi
}

do_init() {
	./service &
	read -p "Display osd & cover [y/n]? >" disp_rgn
	${cmd} w
	if [ "$disp_rgn" == "y" ]; then
		${cmd} region
		${cmd} init
		${cmd} create 0 0 3 200 200
		${cmd} create 1 0 3 200 200
		${cmd} create 2 1 0 0 0
		${cmd} create 3 1 0 0 0
		${cmd} create 4 1 0 0 0
		${cmd} create 5 1 0 0 0
		${cmd} setcanvas 0 26 3
		${cmd} setcanvas 1 26 3
		${cmd} q
	fi
}
do_deinit() {
	if [ "$disp_rgn" == "y" ]; then
		${cmd} region
		${cmd} destroy 0
		${cmd} destroy 1
		${cmd} destroy 2
		${cmd} destroy 3
		${cmd} destroy 4
		${cmd} destroy 5
		${cmd} deinit
		${cmd} q
	fi
	${cmd} q
}
do_region_osd() {
	for port in ${port_array}
	do
		if [ "$disp_rgn" == "y" ]; then
			if [ "${port}" == "2" ]; then
				${cmd} region
				${cmd} attachosd 0 1 0 ${port} 0 1 0 0 0 0 0 0 0 0
				${cmd} attachosd 1 1 0 ${port} 0 1 28 200 1 0 0 0 0 0
				${cmd} q
			else
				${cmd} region
				${cmd} attachosd 0 0 0 0 ${port} 1 0 0 0 0 0 0 0 0
				${cmd} attachosd 1 0 0 0 ${port} 1 28 200 1 0 0 0 0 0
				${cmd} q
			fi
		fi
	done
}
do_region_cover() {
	for port in ${port_array}
	do
		if [ "$disp_rgn" == "y" ]; then
			${cmd} region
			${cmd} attachcover 2 0 0 0 ${port} 1 1000 1000 0 2000 2000 0xFF
			${cmd} attachcover 3 0 0 0 ${port} 1 1500 1500 1 2500 2500 0xFF00
			${cmd} attachcover 4 0 0 0 ${port} 1 2000 2000 2 3000 1000 0xFF0000
			${cmd} attachcover 5 0 0 0 ${port} 1 2500 2500 3 1000 2000 0x0
			${cmd} q
		fi
	done
}

do_rtsp_select() {
	echo "############################################"
	echo "Selected vpe--venc--rtsp case"
	echo "Starting config vpe port${1} and venc output size."
	echo "Type 0 for H265 3840x2160."
	echo "Type 1 for H265 2688x1520."
	echo "Type 2 for H265 2592x1944."
	echo "Type 3 for H265 1920x1080."
	echo "Type 4 for H265 1280x720."
	echo "Type 5 for H265 720x576."
	echo "Type 6 for H265 640x480."
	echo "Type 7 for H265 352x288."
	echo "Type 8 for H264 3840x2160."
	echo "Type 9 for H264 2688x1520."
	echo "Type 10 for H264 2592x1944."
	echo "Type 11 for H264 1920x1080."
	echo "Type 12 for H264 1280x720."
	echo "Type 13 for H264 720x576."
	echo "Type 14 for H264 640x480."
	echo "Type 15 for H264 352x288."
	echo "Type 16 for MJPEG 3840x2160."
	echo "Type 17 for MJPEG 2688x1520."
	echo "Type 18 for MJPEG 2592x1944."
	echo "Type 19 for MJPEG 1920x1080."
	echo "Type 20 for MJPEG 1280x720."
	echo "Type 21 for MJPEG 720x576."
	echo "Type 22 for MJPEG 640x480."
	echo "Type 23 for MJPEG 352x288."
	echo "Type 'Enter' to ignore."
	read -p "Please select:" select
	case ${select} in
	0)
		start_rtsp ${1} 3840 2160 2
	;;
	1)
		start_rtsp ${1} 2688 1520 2
	;;
	2)
		start_rtsp ${1} 2592 1944 2
	;;
	3)
		start_rtsp ${1} 1920 1080 2
	;;
	4)
		start_rtsp ${1} 1280 720 2
	;;
	5)
		start_rtsp ${1} 720 576 2
	;;
	6)
		start_rtsp ${1} 640 480 2
	;;
	7)
		start_rtsp ${1} 352 288 2
	;;
	8)
		start_rtsp ${1} 3840 2160 1
	;;
	9)
		start_rtsp ${1} 2688 1520 1
	;;
	10)
		start_rtsp ${1} 2592 1944 1
	;;
	11)
		start_rtsp ${1} 1920 1080 1
	;;
	12)
		start_rtsp ${1} 1280 720 1
	;;
	13)
		start_rtsp ${1} 1280 720 1
	;;
	14)
		start_rtsp ${1} 640 480 1
	;;
	15)
		start_rtsp ${1} 352 288 1
	;;
	16)
		start_rtsp ${1} 3840 2160 0
	;;
	17)
		start_rtsp ${1} 2688 1520 0
	;;
	18)
		start_rtsp ${1} 2592 1944 0
	;;
	19)
		start_rtsp ${1} 1920 1080 0
	;;
	20)
		start_rtsp ${1} 1280 720 0
	;;
	21)
		start_rtsp ${1} 1280 720 0
	;;
	22)
		start_rtsp ${1} 640 480 0
	;;
	23)
		start_rtsp ${1} 352 288 0
	;;
	*)
		echo "Ignore !!!!"
		return
	;;
	esac
	do_choice_port_on ${1} ${2}
}
do_rtsp_dip_select() {
	echo "############################################"
	echo "Selected vpe--dip--venc--rtsp case"
	echo "Starting config vpe port${1} and venc output size."
	echo "Type 0 for H265 3840x2160."
	echo "Type 1 for H265 2592x1944."
	echo "Type 2 for H265 1920x1080."
	echo "Type 3 for H265 1280x720."
	echo "Type 4 for H265 720x576."
	echo "Type 5 for H265 640x480."
	echo "Type 6 for H265 352x288."
	echo "Type 7 for H264 3840x2160."
	echo "Type 8 for H264 2592x1944."
	echo "Type 9 for H264 1920x1080."
	echo "Type 10 for H264 1280x720."
	echo "Type 11 for H264 720x576."
	echo "Type 12 for H264 640x480."
	echo "Type 13 for H264 352x288."
	echo "Type 14 for MJPEG 3840x2160."
	echo "Type 15 for MJPEG 2592x1944."
	echo "Type 16 for MJPEG 1920x1080."
	echo "Type 17 for MJPEG 1280x720."
	echo "Type 18 for MJPEG 720x576."
	echo "Type 19 for MJPEG 640x480."
	echo "Type 20 for MJPEG 352x288."
	echo "Type 'Enter' to ignore."
	read -p "Please select:" select
	case ${select} in
	0)
		start_rtsp_with_dip ${1} 3840 2160 2
	;;
	1)
		start_rtsp_with_dip ${1} 2592 1944 2
	;;
	2)
		start_rtsp_with_dip ${1} 1920 1080 2
	;;
	3)
		start_rtsp_with_dip ${1} 1280 720 2
	;;
	4)
		start_rtsp_with_dip ${1} 720 576 2
	;;
	5)
		start_rtsp_with_dip ${1} 640 480 2
	;;
	6)
		start_rtsp_with_dip ${1} 352 288 2
	;;
	7)
		start_rtsp_with_dip ${1} 3840 2160 1
	;;
	8)
		start_rtsp_with_dip ${1} 2592 1944 1
	;;
	9)
		start_rtsp_with_dip ${1} 1920 1080 1
	;;
	10)
		start_rtsp_with_dip ${1} 1280 720 1
	;;
	11)
		start_rtsp_with_dip ${1} 720 576 1
	;;
	12)
		start_rtsp_with_dip ${1} 640 480 1
	;;
	13)
		start_rtsp_with_dip ${1} 352 288 1
	;;
	14)
		start_rtsp_with_dip ${1} 3840 2160 0
	;;
	15)
		start_rtsp_with_dip ${1} 2592 1944 0
	;;
	16)
		start_rtsp_with_dip ${1} 1920 1080 0
	;;
	17)
		start_rtsp_with_dip ${1} 1280 720 0
	;;
	18)
		start_rtsp_with_dip ${1} 720 576 0
	;;
	19)
		start_rtsp_with_dip ${1} 640 480 0
	;;
	20)
		start_rtsp_with_dip ${1} 352 288 0
	;;
	*)
		echo "Ignore !!!!"
		return
	;;
	esac
	do_choice_port_on ${1} ${2}
}
do_jpeg_capture() {
	echo "############################################"
	echo "Which port do you want to do capture jpeg?"
	read -p "Please choose port:" port
	if [ ! "${port}" ]; then
		return
	fi
	if [  ${port} -lt 0 -o ${port} -gt 3 ]; then
		echo "Error port number."
		return
	fi
	echo "############################################"
	echo "Selected vpe--venc capture jpeg case"
	echo "Starting config vpe port${1} and venc output size."
	echo "Type 0 for jpeg 3840x2160."
	echo "Type 1 for jpeg 1920x1080."
	echo "Type 2 for jpeg 1280x720."
	echo "Type 3 for jpeg 720x576."
	echo "Type 4 for jpeg 640x480."
	echo "Type 5 for jpeg 352x288."
	echo "Type 6 for jpeg 2592x1944."
	echo "Type 'Enter' to ignore."
	read -p "Please select:" select
	case ${select} in
	0)
		do_rtsp_capture_jpeg ${port} 3840 2160 
	;;
	1)
		do_rtsp_capture_jpeg ${port} 1920 1080
	;;
	2)
		do_rtsp_capture_jpeg ${port} 1280 720
	;;
	3)
		do_rtsp_capture_jpeg ${port} 720 576
	;;
	4)
		do_rtsp_capture_jpeg ${port} 640 480
	;;
	5)
		do_rtsp_capture_jpeg ${port} 352 288
	;;
	6)
		do_rtsp_capture_jpeg ${port} 2592 1944
	;;
	*)
		echo "Ignore !!!!"
	;;
	esac
}
do_baseinit() {
	${cmd} integrate
	echo "############################################"
	echo "Selected vif--vpe case"
	echo "Starting config vpeRunMode."
	echo "Type 0 for Realtime."
	echo "Type 1 for RealTime Top."
	echo "Type 2 for Camera Frame."
	echo "Type 3 for Camera Frame Top."
	read -p "Please select:" RunMode
	case ${RunMode} in
	0)
		vpeworkmode=0x18
		vifworkmode=0
	;;
	1)
		vpeworkmode=0x08
		vifworkmode=0
	;;
	2)
		vpeworkmode=0x06
		vifworkmode=1
	;;
	3)
		vpeworkmode=0x02
		vifworkmode=1
	;;
	*)
		echo "Use default Realtime"
		vpeworkmode=0x18
		vifworkmode=0
	;;
	esac
	echo "Starting config vpeInput size."
	echo "Type 0 for 3840x2160."
	echo "Type 1 for 2688x1520."
	echo "Type 2 for 2592x1944."
	echo "Type 3 for 1920x1080."
	echo "Type 4 for 1280x720."
	echo "Type 5 for 720x576."
	echo "Type 6 for 640x480."
	echo "Type 7 for 352x288."
	echo "Type 'Enter' to ignore."
	read -p "Please select:" select
	case ${select} in
	0)
			vifwidth=3840
			vifheight=2160
	;;
	1)
			vifwidth=2688
			vifheight=1520
	;;
	2)
			vifwidth=2592
			vifheight=1944
	;;
	3)
			vifwidth=1920
			vifheight=1080
	;;
	4)
			vifwidth=1280
			vifheight=720
	;;
	5)
			vifwidth=720
			vifheight=576
	;;
	6)
			vifwidth=640
			vifheight=480
	;;
	7)
			vifwidth=352
			vifheight=288
	;;
	*)
		echo "Use default 1920x1080"
			vifwidth=1920
			vifheight=1080
	;;
	esac
	
	echo "############################################"
	echo "Selected vif framerate."
	read -p "Please set:" src_fps
	if [ "${src_fps}" == "" ]; then
		src_fps="30"
		echo "Default use 30fps"
	fi
	echo "############################################"
	echo "Selected vpe framerate."
	read -p "Please set:" dst_fps
	if [ "${dst_fps}" == "" ]; then
		dst_fps="30"
		echo "Default use 30fps"
	fi
	
		${cmd} vifinit ${vifwidth} ${vifheight} ${vifworkmode}
	
	if [ "${vpeworkmode}" == "0x18" ]; then
		${cmd} vpeinit 0 ${vifwidth} ${vifheight} ${vpeworkmode}
		${cmd} iqopenserver ${vifheight} ${vifheight} 0
		${cmd} setbind2 6 0 0 0 ${src_fps} 7 0 0 0 ${dst_fps} 0x4 0
		echo "vpe init realtime mode"
	fi
	
	if [ "${vpeworkmode}" == "0x08" ]; then
		${cmd} vpeinit 1 ${vifwidth} ${vifheight} ${vpeworkmode}
		${cmd} vpecreateport 1 0 ${vifwidth} ${vifheight} 11
		${cmd} setbind2 6 0 0 0 ${src_fps} 7 0 1 0 ${dst_fps} 0x4 0
		
		${cmd} iqopenserver ${vifheight} ${vifheight} 1	
		
		${cmd} vpeinit 0 ${vifwidth} ${vifheight} 0x10
		${cmd} setbind 7 0 1 0 ${src_fps} 7 0 0 0 ${dst_fps}
		echo "vpe init top-bottom mode"
	fi
	
	${cmd} q
}
do_basedeinit() {
	${cmd} integrate
	if [ "${vpeworkmode}" == "0x18" ]; then
		${cmd} setunbind 6 0 0 0 7 0 0 0
		${cmd} vpedeinit 0
	fi
	
	if [ "${vpeworkmode}" == "0x08" ]; then
		${cmd} setunbind 7 0 1 0 7 0 0 0
		${cmd} vpedeinit 0
		${cmd} vpedestroyport 0 1
		${cmd} vpedeinit 1
		${cmd} setunbind 6 0 0 0 7 0 1 0
		
	fi
	${cmd} iqcloseserver
	${cmd} vifdeinit
	
	${cmd} q
}
do_something() {
	do_region_osd
	do_region_cover
	while true
	do
		for port in ${port_array}
		do
			echo "############################################"
			echo "Config port${port} case"
			echo "Type 0 for vpe--venc--rtsp case"
			echo "Type 1 for vpe--dip--venc--rtsp case"
			read -p "Please select:" select
			case ${select} in
			0)
				do_rtsp_select ${port} ${select}
			;;
			1)
				do_rtsp_dip_select ${port} ${select}
			;;
			*)
				echo "Ignore !!!!"
			;;
			esac
		done
		b_ret=""
		while [ "$b_ret" != "r" -a "$b_ret" != "q" ]; do
			do_jpeg_capture
			echo "################################################"
			echo "Press 'r' to reset."
			echo "Press 'q' to exit."
			read -p "Press 'enter' to continue capture." b_ret
		done
		for port in ${port_array}
		do
			do_choice_port_off ${port}
		done
		if [ "$b_ret" == "q" ]; then
			return
		fi
	done
}
export PATH=${PATH}:/config/mdb/
check_rec
do_init
do_baseinit
do_something
do_basedeinit
do_deinit