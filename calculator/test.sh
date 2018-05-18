#!/bin/bash
sudo rmmod calc
make clean
make
sudo insmod calc.ko
sudo chmod 0666 /dev/calcchar
#echo -n 'testing 123' > /dev/calcchar
#cat /dev/calcchar
#sudo rmmod calc
