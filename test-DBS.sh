xterm -e "./bin/splitter --splitter_port 8001 --source_addr 150.214.150.68 --source_port 4551 --channel BBB-134.ogv --header_size 30000" &
xterm -e "./bin/peer --splitter_addr 127.0.0.1 --splitter_port 8001 --player_port 9999" &
vlc http://localhost:9999 &

