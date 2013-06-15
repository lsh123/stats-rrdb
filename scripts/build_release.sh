#!/bin/sh

NAME=stats-rrdb
BUILD_ROOT=/tmp/$NAME.`date`
GIT_URL="https://github.com/lsh123/$NAME.git"
PWD=`pwd`

echo "=== Starting build in '$BUILD_ROOT'"
mkdir -p "$BUILD_ROOT"
cd "$BUILD_ROOT"

echo "=== Puling sources"
git clone "$GIT_URL"
cd $NAME
find . -name ".git" | xargs rm -rf

echo "=== Configure"
./autogen.sh --prefix=/usr
make dist
cp $NAME*.tar.gz "$PWD"

echo "=== Cleanup"
cd "$PWD"
rm -rf "$BUILD_ROOT"

