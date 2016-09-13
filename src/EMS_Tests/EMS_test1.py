#!/usr/bin/env python

import re
import sys

#The Test Rationale is simple, check if all members of the team are sending/receiving chunks as they should
# from every other member of the team, if yes than this mean the team is up and running correctly

print "Running EMS test for both peers behind the same NAT router \n"
# Namespace addresses and ports of the team stored as a dictionary
addresses = dict(pc1="192.168.56.4", pc2="192.168.56.5", nat1="192.168.56.6", splitter="192.168.57.6",
                 monitor="192.168.57.7", nat1_pub="192.168.57.4", p2Port="41666", p1Port="41667", monitorPort="4555",
                 splitterPort="4554")

# check if splitter has right outputs, that it has all correct members of the team

splitterFunctional = False
with open("splitter.log") as f:
    for line in f:
        if re.match(r'.+%(monitor)s.+%(nat1_pub)s.+%(nat1_pub)s.+' % addresses, line, re.M | re.I):
            print "splitter has correct peers list"
            splitterFunctional = True
            break

    if not splitterFunctional: print "splitter does not have correct peers list"

print "\n"

# check if monitor has right outputs, that it has all correct members of the team
monitorFunctional = False
with open("monitor.log") as f:
    for line in f:
        if re.match(r'.+%(nat1_pub)s.+%(nat1_pub)s.+' % addresses, line, re.M | re.I):
            print "monitor has correct peers list"
            monitorFunctional = True
            break

    if not monitorFunctional: print "monitor does not have correct peers list"

print "\n"

# check if peer1 has right outputs, that it has all correct members of the team
peer1Functional = False
with open("peer1.log") as f:
    for line in f:
        if re.match(r'.+%(monitor)s.+%(pc2)s.+' % addresses, line, re.M | re.I):
            print "peer1 has correct peers list"
            peer1Functional = True
            break

    if not peer1Functional: print "peer1 does not have correct peers list"

print "\n"
# check if peer2 has right outputs, that it has all correct members of the team
peer2Functional = False
with open("peer2.log") as f:
    for line in f:
        if re.match(r'.+%(monitor)s.+%(pc1)s.+' % addresses, line, re.M | re.I):
            print "peer2 has correct peers list"
            peer2Functional = True
            break

    if not peer2Functional: print "peer2 does not have correct peers list"
print "\n"


if splitterFunctional and monitorFunctional and peer1Functional and peer2Functional:
    print "EMS team functional"
    sys.exit(0)
else:
    print "EMS team falty"
    sys.exit(1)
