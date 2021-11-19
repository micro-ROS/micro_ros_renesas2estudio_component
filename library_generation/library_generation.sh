#!/bin/bash
set -e

export BASE_PATH=$(pwd)/..

######## Check existing library ########
if [[ -f "$BASE_PATH/libmicroros/libmicroros.a" && ! -f "$USER_COLCON_META" ]]; then
    echo "micro-ROS library found. Skipping..."
    echo "Delete micro_ros_renesas2estudio_component/libmicroros/ for rebuild."
    exit 0
fi

if [ ! -f "$USER_COLCON_META" ]; then
    rm -rf $BASE_PATH/libmicroros
fi

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

######## Add extra packages  ########
pushd extra_packages
    git clone -b ros2 https://github.com/ros2/geometry2
    cp -R geometry2/tf2_msgs tf2_msgs
    rm -rf geometry2
popd

######## Build  ########

if [ ! -f "$BASE_PATH/libmicroros/libmicroros.a" ]; then
    # If library does not exist build it
    make -f libmicroros.mk
else
    # If exists just rebuild
    make -f libmicroros.mk rebuild_metas
fi

######## Generate extra files ########
find $BASE_PATH/libmicroros/micro_ros_src/src \( -name "*.srv" -o -name "*.msg" -o -name "*.action" \) | awk -F"/" '{print $(NF-2)"/"$NF}' > $BASE_PATH/libmicroros/available_ros2_types

echo "" > $BASE_PATH/libmicroros/built_packages
for f in $(find $BASE_PATH/libmicroros/micro_ros_src/src -name .git -type d); do pushd $f > /dev/null; echo $(git config --get remote.origin.url) $(git rev-parse HEAD) >> $BASE_PATH/libmicroros/built_packages; popd > /dev/null; done;
