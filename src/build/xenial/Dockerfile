# XENIAL 64 Bits
FROM ubuntu:16.04

env DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386
RUN apt-get update && apt-get dist-upgrade  -y
RUN apt-get install -y openjdk-8-jre-headless
RUN apt-get install -y openjdk-8-jdk
RUN apt-get install -y maven
RUN apt-get install -y g++-multilib
RUN apt-get install -y libx11-dev libsoup2.4-dev 
RUN apt-get install -y libselinux1-dev libgtk-3-dev libffi-dev
RUN apt-get install -y openjdk-8-jdk
RUN apt-get install -y libpam-dev libssl-dev libpcsclite-dev 
RUN apt-get install -y libgtk2.0-dev
RUN apt-get install -y libkrb5-dev
RUN apt-get install -y lsb-release

COPY xenial/pom.xml /root/pom.xml
COPY install.sh /root/install.sh
RUN chmod a+x /root/install.sh
COPY settings.xml /root/.m2/settings.xml
ENTRYPOINT /root/install.sh
