FROM ubuntu:18.04

MAINTAINER Alex Couture-Beil <acouturebeil@digitalocean.com>

RUN apt-get update && \
	apt-get install -y build-essential clang-format-7 libcunit1-dev libmicrohttpd-dev libssl-dev git curl && \
	apt-get clean

ENTRYPOINT ["/bin/bash"]
