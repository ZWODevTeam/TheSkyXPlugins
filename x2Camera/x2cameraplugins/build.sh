#!/bin/sh
sub=libusb
a=`otool -L ../../../linux/lib/mac32/libASICamera2.dylib`
#echo $a
if [[ $a =~ $sub ]]; then
echo "$sub is substring of a"
else
echo "$sub is not substring of a"
fi
