#!/bin/bash -e

pushd ../
image=$(mayhem docker-registry)/makesoftwaresafe/toyota-multimedia-2022/atftpd:latest
docker build -t $image -f mayhem/Dockerfile .
docker push $image
popd
mayhem run .
