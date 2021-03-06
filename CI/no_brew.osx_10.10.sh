#!/bin/sh

export CXX=clang++
export CC=clang

DEPENDENCIES_ROOT="~/dev/openmw-deps"
#QT_PATH="~/Qt5.5.0/5.5/clang_64/lib/cmake"
FFMPEG_HOME="/opt/local"

mkdir build
cd build

cmake \
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="10.9" \
-D CMAKE_OSX_SYSROOT="macosx10.11" \
-D CMAKE_BUILD_TYPE=RelWithDebInfo \
-D OPENMW_OSX_DEPLOYMENT=TRUE \
-D DESIRED_QT_VERSION=4.8 \
-D BUILD_ESMTOOL=FALSE \
-D BUILD_MYGUI_PLUGIN=FALSE \
-G"Unix Makefiles" \
..
