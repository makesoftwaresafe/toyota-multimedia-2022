#!/bin/bash

pushd ../
image=$(mayhem docker-registry)/makesoftwaresafe/toyota-multimedia-2022/ntfs-3g:latest
docker build -t $image -f mayhem/Dockerfile .
docker push $image
popd
mayhem run .
