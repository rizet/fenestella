#!/bin/bash
## Mostly AI generated

# Check if correct number of arguments are provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <filename> <directory> <filesize_MB>"
    exit 1
fi

filename="$1"
directory="$2"
filesize_MB="$3"

# Create FAT32 image
dd if=/dev/zero of="$filename" bs=1M count="$filesize_MB" > /dev/null 2>&1
mformat -i "$filename" -F -v "FAT32_IMAGE" :: > /dev/null 2>&1
mcopy -i "$filename" -s -v "$directory"/* ::/ > /dev/null 2>&1

echo "    [fatten] FAT32 image '$filename' generated from '$directory'."
