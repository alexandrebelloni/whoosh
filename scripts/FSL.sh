#!/bin/bash

# whoosh.bin is our bootloader
# boot.img is the concatenation of whoosh and the Linux Kernel
# boot.imx is the final image (boot.img with i.MX Header)

USAGE="$0 -k <kernel Image> -m <path to mkimage> [mmcdevice]"

while getopts hk:m: OPT; do
    case "$OPT" in
        h)
            echo $USAGE
            exit 0
            ;;
        k)
            KERNEL=$OPTARG
            ;;
		m)
			MKIMAGE=$OPTARG
			;;
        \?)
            # getopts issues an error message
            echo $USAGE >&2
            exit 1
            ;;
    esac
done

shift $(expr $OPTIND - 1)

DEVICE=$1

if [ -z "$KERNEL" -o -z "$MKIMAGE" ]; then
	echo $USAGE
	exit 1
fi

# Make sure we are not trying to write to the primary partition
if [ "$1" = "/dev/sda" ]; then
	echo "ERROR: Forbidden to write to /dev/sda"
	echo $USAGE
	exit 1
fi

# Build the bootloader
make clean
make
if [ $? -ne 0 ]; then
	echo "Compilation failed"
	exit 1
fi

# Dynamic calculation of the start address
mkdir out
cat whoosh.bin ${KERNEL} > out/boot.img

FILESIZE=$(stat -c%s "whoosh.bin")
TEMP_START_ADDR=`echo $((16#10008000 - $FILESIZE))`
START_ADDR=`echo "obase=16; $TEMP_START_ADDR" | bc`

# Generate the images and copy it to the sd card
${MKIMAGE} -n $(dirname ${MKIMAGE})/imximage.cfg -T imximage -e $START_ADDR -d out/boot.img out/boot.imx

if [ -n "$DEVICE" ]; then
	sudo dd if=out/boot.imx of=$1 bs=512 seek=2
	sudo sync
fi

