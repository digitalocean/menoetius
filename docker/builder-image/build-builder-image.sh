#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
cd $DIR

COMMIT_HASH=`git rev-parse --short HEAD`
DOCKER_IMAGE="menoetius-builder"
DOCKER_IMAGE_FILE=".docker-image.sh"
echo "export DOCKER_IMAGE=\"${DOCKER_IMAGE}\"" > $DOCKER_IMAGE_FILE
echo "export LATEST_DOCKER_COMMIT_HASH=\"${COMMIT_HASH}\"" >> $DOCKER_IMAGE_FILE
chmod +x $DOCKER_IMAGE_FILE

docker build --network host -t ${DOCKER_IMAGE}:latest -t ${DOCKER_IMAGE}:${COMMIT_HASH} .
