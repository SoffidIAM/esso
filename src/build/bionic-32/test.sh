#!/bin/bash
BASE=$(realpath $(dirname $0)/../../..)
BASE=/home/gbuades/soffid-2.0/esso-enterprise/target/checkout
echo "Base dir $BASE"
docker rm esso-bionic 
docker build -t esso-bionic-32 . &&
docker run -it \
  --name esso-bionic-32 \
  --add-host=forge.dev.lab:10.129.120.2 \
  -v $BASE:/root/src \
  esso-bionic-32
