#!/bin/bash
set -e

export BASE_PATH=$(pwd)/..

######## Check existing library ########
if [ -f "$BASE_PATH/libmicroros/libmicroros.a" ]; then
    echo "micro-ROS library found. Skipping..."
    echo "Delete micro_ros_renesas2estudio_component/libmicroros/ for rebuild."
    exit 0
fi

rm -rf $BASE_PATH/libmicroros

######## Trying to retrieve CFLAGS ########
export RET_CFLAGS=$1

echo "Found CFLAGS:"
echo "-------------"
echo $RET_CFLAGS
echo "-------------"

echo "Using:"
echo "-------------"
echo $(which arm-none-eabi-gcc)
echo Version: $(arm-none-eabi-gcc -dumpversion)
echo "-------------"


######## Build  ########
make -f libmicroros.mk

######## Generate extra files ########
find $BASE_PATH/libmicroros/micro_ros_src/src \( -name "*.srv" -o -name "*.msg" -o -name "*.action" \) | awk -F"/" '{print $(NF-2)"/"$NF}' > $BASE_PATH/libmicroros/available_ros2_types

echo "" > $BASE_PATH/libmicroros/built_packages
for f in $(find $BASE_PATH/libmicroros/micro_ros_src/src -name .git -type d); do pushd $f > /dev/null; echo $(git config --get remote.origin.url) $(git rev-parse HEAD) >> $BASE_PATH/libmicroros/built_packages; popd > /dev/null; done;