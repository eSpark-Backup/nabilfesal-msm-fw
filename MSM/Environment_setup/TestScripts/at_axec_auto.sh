#!/bin/sh
### BEGIN INIT INFO
# Provides:          myscript
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Run my binary every 60 seconds
### END INIT INFO

DAEMON=/MSM/potential-pancake/MSM/bin/at_axec/at_axec   # path to your binary
PIDFILE=/var/run/myscript.pid

case "$1" in
  start)
    echo "Starting myscript..."
    if [ -f "$PIDFILE" ]; then
        echo "Already running."
        exit 1
    fi
    (
      while true; do
        $DAEMON
        sleep 60
      done
    ) &
    echo $! > "$PIDFILE"
    ;;
  stop)
    echo "Stopping myscript..."
    if [ -f "$PIDFILE" ]; then
        kill "$(cat $PIDFILE)" 2>/dev/null
        rm -f "$PIDFILE"
    else
        echo "Not running."
    fi
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  status)
    if [ -f "$PIDFILE" ]; then
        echo "Running with PID $(cat $PIDFILE)"
    else
        echo "Not running"
    fi
    ;;
  *)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 1
    ;;
esac

exit 0
