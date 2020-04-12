#!/bin/bash
PWD=`pwd`
ALKAID_PROJECT_PATH=${PWD}/../../../../../project
TOOLCHAIN_VERSION=`cat ${ALKAID_PROJECT_PATH}/configs/current.configs | awk '/TOOLCHAIN_VERSION/ {print substr($3,$1)}'`
ALKAID_PROJECT_PATH=$(echo "${ALKAID_PROJECT_PATH}" | sed  's/\//\\\//g' )

sed -i "s/set(PROJECT_PTH .*/set(PROJECT_PTH ${ALKAID_PROJECT_PATH})/g" CMakeLists.txt
sed -i "s/set(CROSS_COMPILER_VERSION .*/set(CROSS_COMPILER_VERSION ${TOOLCHAIN_VERSION})/g" CMakeLists.txt

mkdir -p build
pushd build
cmake ..
make
popd

