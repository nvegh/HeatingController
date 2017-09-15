#!/bin/bash
while true; 
do
    T1=$(python /home/pi/MAX6675.py 0)
    T2=$(python /home/pi/MAX6675.py 1)
    echo $(date +"%T")': '$T1'-'$T2
    curl -d 'temp1='$T1'&temp2='$T2 -s -X POST http://nvegh.azurewebsites.net/home/save #>/dev/null
    printf "\n"
    sleep $((5 * 60))
done

# add these lines to /etc/rc.local
# sudo pigpiod
# sudo screen -dm -S templog /home/pi/logTemp.sh

# to reattach screen:
# sudo screen -dr templog