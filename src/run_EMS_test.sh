#!/bin/bash

#This script needs to be ran as root
#directories of key files, be sure to give full path
video="/home/kevin/p2psp/Test/bin/Big_Buck_Bunny_small.ogv"
executables="/home/kevin/p2psp/Test/bin"
virtualnet="/home/kevin/Desktop"
EMS_test="/home/kevin/PycharmProjects/p2psptest"

sudo bash $virtualnet/EMS_virtualnet_setup.sh

cvlc $video --sout "#duplicate{dst=standard{mux=ogg,dst=:8080/BBB-134.ogv,access=http}}" :sout-keep &


sudo xterm -e script -c "ip netns exec splitter $executables/splitter --source_addr 192.168.57.1 --source_port 8080 --team_port 4554 --channel BBB-134.ogv" $EMS_test/splitter.log &

sudo xterm -e script -c "ip netns exec monitor $executables/peer --monitor --splitter_addr 192.168.57.6 --splitter_port 4554 --player_port 9999 --team_port 4555" $EMS_test/monitor.log &

sudo xterm -e script -c "ip netns exec pc1 $executables/peer --player_port 9999 --splitter_addr 192.168.57.6 --splitter_port 4554 --team_port 41667 " $EMS_test/peer1.log &


sudo xterm -e script -c "ip netns exec pc2 $executables/peer --player_port 9999 --splitter_addr 192.168.57.6 --splitter_port 4554 --team_port 41666" $EMS_test/peer2.log &

sleep 3
xterm -e "netcat 192.168.57.7 9999 2>/dev/null >/dev/null" &
sleep 3
xterm -e "netcat 192.168.56.4 9999 2>/dev/null >/dev/null" &
sleep 3
xterm -e "netcat 192.168.56.5 9999 2>/dev/null >/dev/null" &
sleep 10



sudo killall xterm
sudo killall vlc

sleep 1

sudo bash $virtualnet/cleanup_EMS_virtualNet.sh

sleep 1

cd $EMS_test/
python $EMS_test/EMS_test.py
