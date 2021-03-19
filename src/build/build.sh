#!/bin/bash
if [[ "$1" == "" ]]
then
  echo "Enter the distribution name"
  exit 1
fi
if [[ "$2" == "" ]]
then
   BASE=$(realpath $(dirname $0)/../..)
else
   BASE="$2"
fi
echo "Base dir $BASE"
docker rm esso-$1 
docker build -t esso-$1 -f $1/Dockerfile . &&
docker run -it \
  --name esso-$1 \
  --add-host=forge.dev.lab:10.129.120.2 \
  -v $BASE:/root/src \
  esso-$1 
