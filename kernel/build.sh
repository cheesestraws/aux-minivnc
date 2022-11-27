#!/bin/sh

DIR=`dirname $0`
cd $DIR

# compile it up
cd src
rm fb.o 2> /dev/null
cc -c fb.c
mv fb.o ../etc/install.d/boot.d/fb
 
