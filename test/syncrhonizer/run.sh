#! /bin/bash
set -x
if [ ! -f bin/splitter ] || [ ! -f bin/monitor ] || [ ! -f bin/synchronizer ]
	then
	./make.py
fi
./run_a_source.sh
sleep 2
echo "Started source"
echo "Starting splitter"
../../bin/splitter --source_port 8080 --channel test.ts >/dev/null 2>&1 &
../../bin/splitter --source_port 8080 --channel test.ts --splitter_port 8002 >/dev/null 2>&1 &
echo "Starting monitor"
../../bin/monitor --splitter_port 8001  >/dev/null 2>&1 &
../../bin/monitor --splitter_port 8002 --player_port 10000   >/dev/null 2>&1 &
sleep 2
echo "Starting synchronizer"
../../bin/synchronizer --peers 127.0.0.1:9999 127.0.0.1:10000 &
sleep 5
vlc http://localhost:15000 >/dev/null 2>&1 &
echo "PID $$ to kill this script"
PID=$!
wait $PID
kill $(jobs -p)
