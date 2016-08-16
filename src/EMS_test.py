#!/usr/bin/env python

import re
import sys

#The Test Rationale is simple, check if all members of the team are sending/receiving chunks as they should
# from every other member of the team, if yes than this mean the team is up and running correctly

# check if splitter has right outputs, that it is sending chunks to monitor and both peers
splitterFunctional = False
with open("splitter.log") as f:
    monitor = False
    peer1 = False
    peer2 = False
    for line in f:
        if  monitor and peer1 and peer2:
            splitterFunctional = True
            break
        if not monitor :
            if "-> 192.168.57.7:4555" in line:
                print "splitter sending chunks to monitor"
                monitor = True
        if not peer1 :
            if "-> 192.168.57.4:41667" in line:
                print "splitter sending chunks to peer1"
                peer1 = True
        if not peer2 :
            if "-> 192.168.57.4:41666" in line:
                print "splitter sending chunks to peer2"
                peer2 = True

    if not monitor:
        print "splitter not sending chunks to monitor"
    if not peer1:
        print "splitter not sending chunks to peer1"
    if not peer2:
        print "splitter not sending chunks to peer2"


print "\n"

# check if monitor has right outputs, that it is recieving chunks from splitter and both peers
monitorFunctional = False
with open("monitor.log") as f:
    splitter = False
    peer1 = False
    peer2 = False
    for line in f:
        if  splitter and peer1 and peer2:
            monitorFunctional = True
            break
        if not splitter :
            if re.match( r'.+\(0.0.0.0,4555\)<-\d+-\(192.168.57.6,4554\).+', line, re.M|re.I):
                print "monitor receiving chunks from splitter"
                splitter = True
        if not peer1 :
            if re.match( r'.+\(0.0.0.0,4555\)<-\d+-\(192.168.57.4,41667\).+', line, re.M|re.I):
                print "monitor receiving chunks from peer1"
                peer1 = True
        if not peer2 :
            if re.match( r'.+\(0.0.0.0,4555\)<-\d+-\(192.168.57.4,41666\).+', line, re.M|re.I):
                print "monitor receiving chunks from peer2"
                peer2 = True

    if not splitter:
        print "monitor not receiving chunks from splitter"
    if not peer1:
        print "monitor not receiving chunks from peer1"
    if not peer2:
        print "monitor not receiving chunks from peer2"

print "\n"

# check if peer1 has right outputs, that it is recieving chunks from splitter and both peer2 + monitor
peer1Functional = False
with open("peer1.log") as f:
    splitter = False
    monitor = False
    peer2 = False
    for line in f:
        if  splitter and monitor and peer2:
            peer1Functional = True
            break
        if not splitter :
            if re.match( r'.+\(0.0.0.0,41667\)<-\d+-\(192.168.57.6,4554\).+', line, re.M|re.I):
                print "peer1 receiving chunks from splitter"
                splitter = True
        if not monitor :
            if re.match( r'.+\(0.0.0.0,41667\)<-\d+-\(192.168.57.7,4555\).+', line, re.M|re.I):
                print "peer1 receiving chunks from monitor"
                monitor = True
        if not peer2 :
            if re.match( r'.+\(0.0.0.0,41667\)<-\d+-\(192.168.56.5,41666\).+', line, re.M|re.I):
                print "peer1 receiving chunks from peer2"
                peer2 = True

    if not splitter:
        print "peer1 not receiving chunks from splitter"
    if not monitor:
        print "peer1 not receiving chunks from monitor"
    if not peer2:
        print "peer1 not receiving chunks from peer2"

print "\n"

# check if peer2 has right outputs, that it is recieving chunks from splitter and both peer1 + monitor
peer2Functional = False
with open("peer2.log") as f:
    splitter = False
    monitor = False
    peer1 = False
    for line in f:
        if  splitter and monitor and peer1:
            peer2Functional = True
            break
        if not splitter :
            if re.match( r'.+\(0.0.0.0,41666\)<-\d+-\(192.168.57.6,4554\).+', line, re.M|re.I):
                print "peer2 receiving chunks from splitter"
                splitter = True
        if not monitor :
            if re.match( r'.+\(0.0.0.0,41666\)<-\d+-\(192.168.57.7,4555\).+', line, re.M|re.I):
                print "peer2 receiving chunks from monitor"
                monitor = True
        if not peer1 :
            if re.match( r'.+\(0.0.0.0,41666\)<-\d+-\(192.168.56.4,41667\).+', line, re.M|re.I):
                print "peer2 receiving chunks from peer1"
                peer1 = True

    if not splitter:
        print "peer2 not receiving chunks from splitter"
    if not monitor:
        print "peer2 not receiving chunks from monitor"
    if not peer1:
        print "peer2 not receiving chunks from peer1"

print "\n"


if splitterFunctional and monitorFunctional and peer1Functional and peer2Functional:
    print "EMS team functional"
    sys.exit(0)
else:
    print "EMS team falty"
    sys.exit(1)
