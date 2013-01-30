#!/bin/bash

TPATH=/opt/arm

ques()
{
 echo "Q1:  What type of bootloader is lpc21isp?"
 echo "Q2:  What other programmers exists normally"
 echo "Q3:  How to find out what terminal device to use?"
 echo "Q4:  How does normal programming command look"
 echo "Q5:  How do you boot the MCU bootloader"
}

tst()
{
 /opt/arm/bin/lpc21isp  -detectonly  /dev/ttyUSB0 115200  14746
 # lpc21isp version 1.87
 # Synchronizing (ESC to abort). OK
 # Read bootcode version: 2
 # 4
 # Read part ID: LPC1768, 512 kiB ROM / 64 kiB SRAM (0x26013F37)
}

get_install_lpc21isp()
{ 
 git clone https://github.com/capiman/lpc21isp.git
 cd lpc21isp
 make 
 echo "Copying lpc21isp to $TPATH/bin"
 cp lpc21isp $TPATH/bin
}

#get_install_lpc21isp
ques