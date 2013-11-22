#!/bin/sh

answer=$(alert "This will install updated add-ons (and two new ones) for Becasso 1.5." "Cancel" "Continue")
if [ $answer == "Cancel" ]
then
	exit 1;
fi

path=${0%install.sh}
cd $path
unzip addons

if [ $(./cpuid) == PPro ]
then
	rm -r ../add-ons
	mv add-ons.i686 ../add-ons
else
	rm -r ../add-ons
	mv add-ons.i585 ./add-ons
fi

alert "Done."