#!/bin/zsh
#!/opt/homebrew/bin/python3.13

set -x

rm /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT-programmer/firmware/microSWIFT_V2.2.elf

if [ "$1" == "debug" ]; then
	cp /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Debug/microSWIFT_V2.2.elf /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT-programmer/firmware

	cd /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT-programmer/
	/opt/homebrew/bin/python3.13 microSWIFT_programmer.py --no_firmware_update --debugger_attached

elif [ "$1" == "deploy" ]; then
	cp /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Deployment/microSWIFT_V2.2.elf /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT-programmer/firmware
	cd /Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT-programmer/
	/opt/homebrew/bin/python3.13 microSWIFT_programmer.py --no_firmware_update

else
	echo "Arg not caputred."

fi
