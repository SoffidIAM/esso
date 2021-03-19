#!/bin/bash
cd /root/
mkdir build
cp -r /root/src/* /root/build
vold=$(fgrep '<version>' /root/pom.xml | head -1)
vnew=$(fgrep '<version>' /root/src/pom.xml | head -1)
echo "Setting $vnew"
sed -e "s|$vold|$vnew|" </root/pom.xml >/root/build/pom.xml
cd build
codename=$(lsb_release -cs)
find . -name pom.xml -print  | while read f
do
  mv $f $f.old
  cat $f.old |
  sed -e 's/com.soffid.iam.esso.l64/com.soffid.iam.esso.l64-'$codename'/g' |
  sed -e 's/<architecture>l64</<architecture>l64-'$codename'</g' |
  sed -e 's/<architecture>l32</<architecture>l32-'$codename'</g' |
  sed -e 's/<architecture>l\${architecture.bits}</<architecture>l\${architecture.bits}-'$codename'</g' |
  sed -e 's/com.soffid.iam.esso.l32/com.soffid.iam.esso.l32-'$codename'/g' > $f
done
if [ -r /root/build/installer-deb/installer-l64/src/main/deb/control-$codename ]
then
  cp /root/build/installer-deb/installer-l64/src/main/deb/control-$codename /root/build/installer-deb/installer-l64/src/main/deb/control
fi
if [ -r /root/build/installer-deb/installer-l32/src/main/deb/control-$codename ]
then
  cp /root/build/installer-deb/installer-l32/src/main/deb/control-$codename /root/build/installer-deb/installer-l64/src/main/deb/control
fi
mvn -f /root/build/pom.xml clean install deploy
cd /root/build
find installer-deb -name \*.deb -print -exec cp '{}' '../src/{}' \;
