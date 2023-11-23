#!/bin/sh

SERVICE_NAME=aesdsocket
SERVICE_PATH=/usr/bin/aesdsocket
SERVICE_ARGS=-d


case "$1" in
    start)
        echo "Starting aesdocket daemon"
        start-stop-daemon -S -n ${SERVICE_NAME} -a "${SERVICE_PATH}" -- ${SERVICE_ARGS}
        ;;

    stop)
        echo "Stopping aesdocket daemon"
        # defaults to SIGTERM
        start-stop-daemon -K -n ${SERVICE_NAME}
        ;;
    *)
        echo "Usage: $0 start|stop"
        exit 1
        ;;
esac

exit 0
