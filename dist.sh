#!/bin/bash

# Copyright (C) 2020 Leseratte
# Copyright (C) 2022 cyberstudio

# This bash script is running inside the Docker container
# to actually compile the cIOS. It first creates the
# stripios binary, then runs the maked2x script, while
# including the git commit ID in the version string.

# See wiki, this is needed to be able to compile outside of Github environment.
cd /docker-mountpoint || /bin/true

cd stripios
g++ main.cpp -o stripios
cp stripios ../source/stripios
cd ..
timestamp=$(git log -1 --pretty=%ct)
echo Timestamp is $timestamp
export SOURCE_DATE_EPOCH=$timestamp 
# The build shall reproduce the exact same binaries no matter who runs it. This
# requires a versioning mechanism that only depends on git HEAD. We use the
# 7-hex-digit commit hash because it is the shortest thing that can be displayed
# on screen that uniquely identifies the build.
MINOR_VERSION=+$(git rev-parse --short HEAD)
export D2XL_VER_COMPILE="v9${MINOR_VERSION}"
./maked2x.sh 9 ${MINOR_VERSION}

