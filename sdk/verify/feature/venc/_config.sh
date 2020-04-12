#!/bin/sh

#==== Set these variable to sync with YOUR EVB ====
#the name of the executables, which are executable without path.
if [ "${exe}" == "" ]; then exe=./feature_venc; fi
if [ "${mnt}" == "" ]; then
  mnt=`mount | grep addr | cut -d' ' -f3`
  if [ -d ${mnt} ]; then
    echo "samba is detected on ${mnt}"
  else
    mnt=/mnt
    echo samba is not detected. set to default path ${mnt}
  fi
fi

#==== The following should be the same for all i2 DVR users ====
#config the variable to sync with Linux environment (/project)
out_path=/tmp
mi_venc=/proc/mi_modules/mi_venc
dbg_level=${mi_venc}/debug_level
result=result

echo 2 > $dbg_level #/sys/module/mi_venc/parameters/debug_level

if [ ! -d $mnt/out/ ]; then mkdir $mnt/out/; fi
if [ ! -d $mnt/out/es ]; then mkdir $mnt/out/es; fi

if [ ! -d $mnt/out/es ]; then
  echo -e "Please check if ${mnt} is a writable path. Set variable \e[31mmnt\e[0m in shell or _config.sh"
  pause "Press enter to continue..."
  return 1./
fi

#output to here for analyzer or MD5 checking
to=$mnt/out/es
unset_script=./_unset_config.sh
#set common file pattern for script set version 2
if [ "${ver}" == "2" ]; then
  of0=${out_path}/enc_c00.es;
  of1=${out_path}/enc_c01.es;
  of2=${out_path}/enc_c02.es;
  of3=${out_path}/enc_c03.es;
  of=$of0
fi

#default answer value: s_ for standard answer, p_ for PMBR OFF.
#Note that prefix means off or a special version of driver setting
if [ "${ans_ver}" == "" ]; then ans_ver=p_; fi #current tip code is PMBR off version.


#set default config
export VENC_GLOB_MAX_FRAMES=5
export VENC_GLOB_DUMP_ES=5
export VENC_GLOB_IN_MSG_LEVEL=0
export VENC_CH00_QP=25
export VENC_GLOB_AUTO_STOP=1
export VENC_GLOB_IN_FPS=30 #1000: asap
export VENC_GLOB_IN_FILE_CACHE_MB=30 #0: for non cache. 30 for FHD 10 frames.
if [ "${ver}" == "2" ]; then
  export VENC_GLOB_OUT_FILE_PATTERN="enc_c%02!.es"
fi

if false ; then
#Set this value align with venc include files
yuv="352x288 5 frames"
#yuv="352x288 10 frames"
#yuv="320x240 10 frames"


#append your answers if needed.
if [ "${yuv}" == "320x240 10 frames" ]; then
md5ch0=4c31debc017d23b2ec80e4868ec9f4b4
md5ch1=06146b0e937279e05fdfc4e80c16d3de
elif [ "${yuv}" = "352x288 5 frames" ]; then
md5ch0=bfbdad3193faea0f7ab4c576e3383e0c
md5ch1=dc27ba616075ad6cf63e0662ec926535
elif [ "${yuv}" = "352x288 10 frames" ]; then
md5ch0=bfbdad3193faea0f7ab4c576e3383e0c
md5ch1=dc27ba616075ad6cf63e0662ec926535
fi

fi