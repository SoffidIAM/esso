#!/bin/bash
cd /root/
mkdir build
cp -r /root/src/* /root/build
cp /root/pom.xml /root/build/pom.xml
cd build
find . -name pom.xml -print  | while read f
do
  mv $f $f.old
  cat $f.old |
  sed -e 's/com.soffid.iam.esso.l64/com.soffid.iam.esso.l64-bionic/g' |
  sed -e 's/<architecture>l64</<architecture>l64-bionic</g' |
  sed -e 's/<architecture>l32</<architecture>l32-bionic</g' |
  sed -e 's/<architecture>l\${architecture.bits}</<architecture>l\${architecture.bits}-bionic</g' |
  sed -e 's/com.soffid.iam.esso.l32/com.soffid.iam.esso.l32-bionic/g' > $f
done
mvn -f /root/build/pom.xml install deploy
