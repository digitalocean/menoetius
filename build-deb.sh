#!/bin/bash
set -e

DOCKER_IMAGE_PATH=docker/builder-image/.docker-image.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
cd $DIR

if [ ! -f $DOCKER_IMAGE_PATH ]; then
	echo Building builder-image container
	docker/builder-image/build-builder-image.sh
fi
. $DOCKER_IMAGE_PATH
docker run --rm --network=host  -ti -v `pwd`:/code -w /code $DOCKER_IMAGE:$LATEST_DOCKER_COMMIT_HASH /code/.build-deb.sh
