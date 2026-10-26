#!/bin/sh
#
# Start/stop aesdsocket daemon
#

DAEMON="aesdsocket"
DAEMON_PATH="/usr/bin/$DAEMON"

case "$1" in
    start)
        echo "Starting $DAEMON"
        start-stop-daemon -S -n "$DAEMON" -a "$DAEMON_PATH" -- -d
        ;;
    stop)
        echo "Stopping $DAEMON"
        start-stop-daemon -K -n "$DAEMON"
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0
