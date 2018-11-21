#!/bin/bash
sudo rmmod calc
make clean
make
sudo insmod calc.ko
sudo chmod 0666 /dev/calc
# testing multiply
echo "Testing: 5*5"
echo -n '5*5\0' > /dev/calc
cat /dev/calc
# testing add
echo "Testing: 256+5"
echo -n '256+5\0' > /dev/calc
cat /dev/calc
# testing sub
echo "Testing: 512-5"
echo -n '512-5\0' > /dev/calc
cat /dev/calc
# testing div
echo "Testing: 25/5"
echo -n '25/5\0' > /dev/calc
cat /dev/calc
sudo rmmod calc
