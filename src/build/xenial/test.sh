#!/bin/bash
BASE=$(realpath $(dirname $0)/../../..)
BASE=/home/gbuades/soffid-2.0/esso-enterprise/target/checkout
echo "Base dir $BASE"
docker rm esso-xenial 
docker build -t esso-xenial . &&
docker run -it \
  --name esso-xenial \
  --add-host=forge.dev.lab:10.129.120.2 \
  -v $BASE:/root/src \
  esso-xenial 
