#!/bin/sh

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

if [ -z "${searchstr}" ]; then
    echo "<searchstr> should not be empty."
    exit 1;
fi

# number of files found
filecount=$(find "${filesdir}" -type f | wc -l)
strcount=$(grep -rh "${searchstr}" "${filesdir}" | wc -l)

echo "The number of files are ${filecount} and the number of matching lines are ${strcount}"
