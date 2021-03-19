# BIONIC 32 Bits
FROM ubuntu:18.04

env DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386
RUN apt-get update && apt-get dist-upgrade  -y
RUN apt-get install -y openjdk-8-jre-headless
RUN apt-get install -y openjdk-8-jdk
RUN apt-get install -y maven 
RUN apt-get install -y g++-multilib libx11-dev:i386 libsoup2.4-dev:i386
RUN apt-get install -y libselinux1-dev:i386 libgtk-3-dev:i386 libffi-dev:i386
RUN apt-get install -y libpam-dev:i386 libssl-dev:i386 libpcsclite-dev:i386
RUN apt-get install -y libgtk2.0-dev:i386
RUN apt-get install -y lsb-release

COPY bionic-32/pom.xml /root/pom.xml
COPY install.sh /root/install.sh
RUN chmod a+x /root/install.sh
COPY settings.xml /root/.m2/settings.xml
ENTRYPOINT /root/install.sh
