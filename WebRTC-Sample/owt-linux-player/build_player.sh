#!/bin/bash -ex

ROOT=`pwd`
WEBRTC_LINUX_CLIENT_SDK=${ROOT}/webrtc_linux_client_sdk/release
PLAYER_BUILD=${ROOT}/player/

# player lib configure
PLAYER_LIB_SRC=${ROOT}/../../src/player/player_lib
PLAYER_LIB_BUILD=${ROOT}/player/player_lib_build
mkdir -p ${PLAYER_LIB_BUILD}
cd ${PLAYER_LIB_BUILD}
cmake -DLINUX_OS=ON -DUSE_OMAF=OFF -DUSE_WEBRTC=ON -DWEBRTC_LINUX_SDK=${WEBRTC_LINUX_CLIENT_SDK} ${PLAYER_LIB_SRC}
make -j

# player app configure
PLAYER_APP_SRC=${ROOT}/../../src/player/app
PLAYER_APP_BUILD=${ROOT}/player/player_app_build
mkdir -p ${PLAYER_APP_BUILD}/
cd ${PLAYER_APP_BUILD}/
cmake -DUSE_OMAF=OFF -DUSE_WEBRTC=ON -DWEBRTC_LINUX_SDK=${WEBRTC_LINUX_CLIENT_SDK} ${PLAYER_APP_SRC}
make -j

#rm -fv ${PLAYER_BUILD}/render
#cp -v ${PLAYER_BUILD}/Build/render ${PLAYER_BUILD}/render

#if [ ! -f ${PLAYER_BUILD}/config.xml ]; then
#    cp -v ${PLAYER_SRC}/config.xml ${PLAYER_BUILD}/config.xml
#    sed 's|<sourceType>.*</sourceType>|<sourceType>2</sourceType>|g' -i ${PLAYER_BUILD}/config.xml

#    sed 's|<resolution>.*</resolution>|<resolution>4k</resolution>|g' -i ${PLAYER_BUILD}/config.xml
#    sed 's|<server_url>.*</server_url>|<server_url>http://owt-server-ip:3001</server_url>|g' -i ${PLAYER_BUILD}/config.xml
#fi


