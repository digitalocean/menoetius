#!/usr/bin/env bash
set -e
set -x
PACKAGE_NAME=menoetius
MAJOR_VERSION=0
MINOR_VERSION=1
BUILD_VERSION=`date +%s`

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

COMMIT=`git rev-parse --short HEAD`
CURRENTBRANCH=`git rev-parse --abbrev-ref HEAD`

make clean
make release GIT_COMMIT=$COMMIT

NAME=${PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}-${BUILD_VERSION}

rm -rf package/*
mkdir -p package/$NAME/usr/local/bin
cp menoetius-server package/$NAME/usr/local/bin/menoetius
cp menoetius-cli package/$NAME/usr/local/bin/menoetius-cli

mkdir -p package/$NAME/etc/systemd/system/
cp systemd-menoetius.service package/$NAME/etc/systemd/system/menoetius.service

cp menoetius.conf package/$NAME/etc/.

mkdir -p package/$NAME/var/local/menoetius

mkdir -p package/$NAME/DEBIAN
cat > package/$NAME/DEBIAN/control <<- EndOfMessage
Package: $PACKAGE_NAME
Version: $MAJOR_VERSION.$MINOR_VERSION-$BUILD_VERSION
Section: base
Priority: optional
Architecture: amd64
Depends: libmicrohttpd12
Maintainer: Alex Couture-Beil <acouturebeil@digitalocean.com>
Description: menoetius
 in memory metrics store
EndOfMessage

cat > package/$NAME/DEBIAN/postinst <<- EndOfMessage
#!/bin/bash
if [ -d /run/systemd/system ]; then
    systemctl --system daemon-reload >/dev/null || true
fi
EndOfMessage
chmod +x package/$NAME/DEBIAN/postinst

cat > package/$NAME/DEBIAN/conffiles <<- EndOfMessage
/etc/menoetius.conf
EndOfMessage

# default -Z xz compression doesnt work with ubuntu 14.04
dpkg-deb -Z gzip --build package/$NAME

echo Finished building package/$NAME
