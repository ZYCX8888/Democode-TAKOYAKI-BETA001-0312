DB_PROJECT=demo
DB_BUILD_TOP=$PWD
DB_OUT_PATH=$DB_BUILD_TOP/out/$DB_PROJECT
DB_LIB_NAME=mdb
DB_LIB_TYPE=static
DB_MAKE="make -\$(MAKEFLAGS)";
DB_TRANS_BUFFER_SIZE=256 #IPC buffer size is 256K
DB_USE_SHM=1
DB_USE_SOCKET=0
DB_ALKAID_PROJ=../../../../../../project/configs/current.configs

export DB_BUILD_TOP
export DB_OUT_PATH
export DB_PROJECT
export DB_LIB_TYPE
export DB_LIB_NAME
export DB_MAKE
export DB_TRANS_BUFFER_SIZE
export DB_USE_SOCKET
export DB_USE_SHM
export DB_ALKAID_PROJ
