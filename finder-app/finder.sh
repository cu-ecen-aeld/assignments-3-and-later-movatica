#!/bin/bash

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1;
fi

filesdir=$1
searchstr=$2

if [ ! -d "${filesdir}" ]; then
    echo "${filesdir} is not a valid directory."
    exit 1;
fi

filelist=()

# get list of files in search directory and all subdirectories
mapfile -d '' filelist < <(find -L "${filesdir}" -type f -print0)

# number of files found
filecount=${#filelist[@]}

strcount=0
for filename in "${filelist[@]}"; do
    # sum the number of 
    strcount=$(( strcount + $(grep -c "${searchstr}" "${filename}") ))
done

echo "The number of files are ${filecount} and the number of matching lines are ${strcount}"
