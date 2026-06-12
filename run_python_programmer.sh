#!/bin/zsh
#!/opt/homebrew/bin/python3.13

set -x

USWIFT_DIR="/Users/lindzey/Documents/Thompson"

rm ${USWIFT_DIR}/microSWIFT-programmer/microSWIFT_Programmer/firmware/microSWIFT_V2.2.elf

if [ "$1" == "debug" ] || [ "$1" == "deploy" ]; then
	cp ${USWIFT_DIR}/microSWIFT_V2.2/microSWIFT_V2.2/Debug/microSWIFT_V2.2.elf ${USWIFT_DIR}/microSWIFT-programmer/microSWIFT_Programmer/firmware
	cd ${USWIFT_DIR}/microSWIFT-programmer/microSWIFT_Programmer/src
	/Users/lindzey/Documents/Thompson/microSWIFT-programmer/.venv/bin/python3 microSWIFT_programmer.py --no_firmware_update

else
	echo "Arg not caputred."

fi
