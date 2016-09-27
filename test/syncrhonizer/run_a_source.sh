#!/bin/bash

export CHANNEL="test.ts"
export SOURCE_PORT=8080
export VIDEO="$HOME/Videos/BigBuckBunny_512kb.mp4"

usage() {
    echo $0
    echo " Run a source."
    echo "  [-c channel ($CHANNEL)]"
    echo "  [-o (listening) port ($PORT)]"
    echo "  [-v video ($VIDEO)]"
    echo "  [-? help]"
}

echo $0: parsing: $@

while getopts "c:o:v:?" opt; do
    case ${opt} in
        c)
            CHANNEL="${OPTARG}"
            echo "CHANNEL="$CHANNEL
            ;;
        o)
            PORT="${OPTARG}"
            echo "PORT="$PORT
            ;;
        v)
            VIDEO="${OPTARG}"
            echo "VIDEO="$VIDEO
            ;;
        ?)
            usage
            exit 0
            ;;
        \?)
            echo "Invalid option: -${OPTARG}" >&2
            usage
            exit 1
            ;;
        :)
            echo "Option -${OPTARG} requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done

set -x

#vlc $VIDEO --sout "#duplicate{dst=standard{mux=ogg,dst=:$PORT/$CHANNEL,access=http}}" &
vlc $VIDEO --sout "#transcode:http{dst=:8080/test.ts}" &
#:sout=#transcode:http{dst=:8080/test.ts}
