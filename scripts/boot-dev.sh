#!/usr/bin/env sh

docker container ls -a | grep chronic | awk '{ print $1 }' | xargs docker rm
docker image ls | grep chronic | awk '{ print $3 }' | xargs docker rmi
docker build -f Dockerfile.dev -t chronic .
docker run -it --entrypoint /bin/bash -v $(pwd):/usr/app chronic
