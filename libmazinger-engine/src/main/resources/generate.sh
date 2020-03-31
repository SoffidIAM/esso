#!/bin/bash
dir=$(dirname $0)
targetdir=$1
echo "$dir ==> $targetdir" 
for i in $dir/*.png
do
   fn=$(basename $i .png)
   mkdir -p $targetdir
   target=$targetdir/img_$fn.h
   echo $target
   echo "static const char *_img_resource_$fn=\"data:image/png;base64,\"" >$target
   openssl base64 -in $i |
   while read line
   do
      echo "	\"$line\"" >> $target
   done
   echo ";"  >>$target
done
