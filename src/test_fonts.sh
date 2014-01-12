#!/bin/bash

show="LCD SHOW"
clear="LCD CLEAR"
text="LCD TEXT %ID% 0 0 Text %ID%"
#text="LCD TEXT %ID% 0 0 Text"

echo $clear | nc -U test.socket
echo $show | nc -U test.socket
for i in `echo 0 2 4 6 8 10 11 12 13 14 15 16 17 18 19 20 21 26 27 30 31 32 33`
do
echo "Text Id $i"
echo $clear | nc -U test.socket
test=`echo $text | sed "s/%ID%/$i/g"`
echo $test | nc -U test.socket
echo $show | nc -U test.socket
sleep 2
done
