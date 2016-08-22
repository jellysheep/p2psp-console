#!/bin/bash

#This script needs to be ran as root
#directories of key files, be sure to give full path
video="/home/kevin/p2psp/Test/bin/Big_Buck_Bunny_small.ogv"
executables="/home/kevin/p2psp/Test/bin"
virtualnet="/home/kevin/Desktop"
EMS_test="/home/kevin/PycharmProjects/p2psptest"


# Namespace addresses
pc1="192.168.56.4"
pc2="192.168.58.5"
nat1="192.168.56.5"
nat2="192.168.58.4"
splitter="192.168.57.6"
monitor="192.168.57.7"
# NAT addresses towards the "internet"
nat1_pub="192.168.57.4"
nat2_pub="192.168.57.5"

monitorPort="4555"
p1Port="41667"
p2Port="41666"
playerPort="9999"
splitterPort="4554"

sourceAddr="192.168.57.1"
sourcePort="8080"
channel="BBB-134.ogv"



bash $virtualnet/EMS_virtualnet_setup2.sh

cvlc $video --sout "#duplicate{dst=standard{mux=ogg,dst=:$sourcePort/$channel,access=http}}" :sout-keep &


xterm -e script -c "ip netns exec splitter $executables/splitter --EMS --source_addr $sourceAddr --source_port $sourcePort --team_port $splitterPort --channel $channel" $EMS_test/splitter.log &

xterm -e script -c "ip netns exec monitor $executables/peer --monitor --splitter_addr $splitter --splitter_port $splitterPort --player_port $playerPort --team_port $monitorPort" $EMS_test/monitor.log &

xterm -e script -c "ip netns exec pc1 $executables/peer --player_port $playerPort --splitter_addr $splitter --splitter_port $splitterPort --team_port $p1Port " $EMS_test/peer1.log &


xterm -e script -c "ip netns exec pc2 $executables/peer --player_port $playerPort --splitter_addr $splitter --splitter_port $splitterPort --team_port $p2Port" $EMS_test/peer2.log &

sleep 3
xterm -e "netcat $monitor $playerPort 2>/dev/null >/dev/null" &
sleep 3
xterm -e "netcat $pc1 $playerPort 2>/dev/null >/dev/null" &
sleep 3
xterm -e "netcat $pc2 $playerPort 2>/dev/null >/dev/null" &
sleep 10



killall xterm
killall vlc

sleep 1

bash $virtualnet/cleanup_EMS_virtualNet2.sh

sleep 1

cd $EMS_test/
python EMS_test2.py
