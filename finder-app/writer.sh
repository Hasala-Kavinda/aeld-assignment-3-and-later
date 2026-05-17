#!/bin/bash


if [ $# -ne 2 ]; then
    echo "Error: Two arguments required."
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
fi

writefile=$1
writestr=$2
 
dirpath=$(dirname "$writefile")


mkdir -p "$dirpath"

if [ $? -ne 0 ]; then
    echo "Error: Could not create directory path $dirpath"
    exit 1
fi


echo -n "$writestr" > "$writefile"

# Alternative if echo -n doesn't work on your system:
# printf "%s" "$writestr" > "$writefile"


if [ $? -ne 0 ]; then
    echo "Error: Could not create file $writefile"
    exit 1
fi


if [ ! -f "$writefile" ]; then
    echo "Error: File $writefile was not created"
    exit 1
fi

exit 0
