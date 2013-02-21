#!/bin/bash

TPATH=/opt/arm

ques()
{
 echo "Q1:  What's the libc used?"
 echo "Q2:  What's hardware/compiler architecure?"
 echo "Q3:  What's the path to binaries?"
 echo "Q4:  What's gcc version for the toolchain?"
 echo "Q5:  What's the basic idea with build script?"
}

get_ubuntu_packages()
{
 apt-get install flex bison libgmp3-dev libmpfr-dev libncurses5-dev 
 apt-get install libmpc‐dev autoconf texinfo build‐essential 
 apt-get install libftdi-dev zlib1g-dev git zlib1g-dev python-yaml 

 # gcc 4.5 needs
 apt-get install libgmp-dev
 apt-get install libmpfr-dev 
 apt-get install libmpc-dev
 apt-get build-dep gcc-4.5

}

get_arm_toolchain()
{
 git clone http://github.com/esden/summon-arm-toolchain.git
}

build_arm_toolchain()
{
 cd summon-arm-toolchain
 git pull 
 ./summon-arm-toolchain PREFIX=$TPATH USE_LINARO=0
 cd -
}

#get_ubuntu_packages
#get_arm_toolchain
build_arm_toolchain

ques