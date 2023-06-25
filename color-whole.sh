#!/bin/bash
last_value=15
counter=0

while [ $counter -le $last_value ]
do
    mosquitto_pub -h 192.168.178.81 -t "/deepdeck/led_colors" -m "220,100,1,$counter,"
    ((counter++))
done
