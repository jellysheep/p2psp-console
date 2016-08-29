# EMS Tests documentation

## How it Works/How to Use it

This test script works by first using network namespaces to create virtual networks to simulate the following 2 scenarios

1. two peers behind the same NAT device ([EMS_virtualnet_setup1.sh] (https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/EMS_virtualnet_setup1.sh))
2. two peers behind different NAT devices ([EMS_virtualnet_setup2.sh] (https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/EMS_virtualnet_setup2.sh))

(all scripts/files corresponding to case 1 are suffixed with 1, and likewise for case 2)

In both cases, the splitter and monitor are at public, non-NATed addresses. For the scripts to work, it must be run as root. Make sure to first place the files [iptables.rules.prcn1](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/iptables.rules.prcn1)  and [iptables.rules.prcn2](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/iptables.rules.prcn2) into the folder expected by the script. Note to ensure that the correct full path is supplied for sshd.

After the virtual networks are set up, the two bash scripts ([run_EMS_test1.sh](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/run_EMS_test1.sh) and [run_EMS_test2.sh](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/run_EMS_test2.sh)) are used to simulate the running of a team (splitter, monitor, 2 peers) using xterm. 
Again the scripts must be run as root. Note to supply the correct full path the the test video file to be streamed, as well as the correct full path directories for the peer and splitter executables, the network setup scripts, and the python test scripts.
These scripts will run the team and deposit the respective output logs from the splitter, monitor and peers to be read by 2 python scripts.

The python scripts [EMS_test1.py](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/EMS_test1.py), [EMS_test2.py](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/EMS_test2.py) will read through the ouput logs to make sure all members of the team are sending/receiving chunks from each other. 
If everything is as expected, it will report that the team is functional. If there is a member of the team not sending/receiving chunks as expected it will be identified and the team reported as faulty.

After the python scripts finished running [cleanup_EMS_virtualNet1.sh](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/cleanup_EMS_virtualNet1.sh) and [cleanup_EMS_virtualNet2.sh](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/cleanup_EMS_virtualNet2.sh) will dismantle/remove the virtual networks.


## To Run the Tests
simply make sure all files/directories are correctly supplied as described above and run [run_EMS_test1.sh](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/run_EMS_test1.sh) or [run_EMS_test2.sh](https://github.com/kshi219/p2psp-console/blob/master/src/EMS_Tests/run_EMS_test2.sh)
as root depending on which case you would like to test.

