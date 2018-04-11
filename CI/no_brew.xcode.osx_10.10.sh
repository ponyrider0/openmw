#!/bin/sh

export CXX=clang++
export CC=clang

export DEPENDENCIES_ROOT="${HOME}/dev/openmw-deps"
export FFMPEG_HOME="/opt/local"
export QT_PATH="/opt/local/libexec/qt4"

mkdir build
cd build

cmake \
-D CMAKE_PREFIX_PATH="$DEPENDENCIES_ROOT;$QT_PATH" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="10.9" \
-D CMAKE_OSX_SYSROOT="macosx10.11" \
-D DESIRED_QT_VERSION=4.8 \
-D BUILD_ESMTOOL=FALSE \
-D BUILD_MYGUI_PLUGIN=FALSE \
-G"Xcode" \
..
