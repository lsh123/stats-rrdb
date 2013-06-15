#!/bin/sh

NAME=stats-rrdb
BUILD_ROOT=/tmp/$NAME.`date  +"%Y-%m-%d.%H:%m:%S"`
GIT_URL="https://github.com/lsh123/$NAME.git"
CUR_PWD=`pwd`
echo "$CUR_PWD"

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
cp $NAME*.tar.gz "$CUR_PWD"

echo "=== Cleanup"
cd "$CUR_PWD"
rm -rf "$BUILD_ROOT"

