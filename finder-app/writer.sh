#!/bin/bash

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1;
fi

writefile=$1
writestr=$2

writedir=$(dirname "${writefile}")

mkdir -p "${writedir}"

echo "${writestr}" >> "${writefile}"

if [ $? -ne 0 ]; then
    echo "Failed to write to ${writefile}"
    exit 1;
fi
