#!/bin/bash
# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# Get the absolute path of the script
abspath=$(cd ${0%/*} && echo $PWD/${0##*/})
scriptdir="`dirname \"$abspath\"`"
clear
CMAKE="$scriptdir/../../tools/cmake/bin/cmake.app/Contents/bin/cmake"
if [ ! -f "$CMAKE" ]
then
echo "ERROR: Cmake tool not present at $CMAKE, cannot proceed "
exit 1
fi
clean()
{
echo "Cleaning..."
if [ -e cmake/CustomSchema/build ]
then
rm -rf cmake/CustomSchema/build
fi
if [ -e cmake/CustomSchema/build_x64 ]
then
rm -rf cmake/CustomSchema/build_x64
fi
if [ -e cmake/DumpFile/build ]
then
rm -rf cmake/DumpFile/build
fi
if [ -e cmake/DumpFile/build_x64 ]
then
rm -rf cmake/DumpFile/build_x64
fi
if [ -e cmake/DumpMainXMP/build ]
then
rm -rf cmake/DumpMainXMP/build
fi
if [ -e cmake/DumpMainXMP/build_x64 ]
then
rm -rf cmake/DumpMainXMP/build_x64
fi
if [ -e cmake/DumpScannedXMP/build ]
then
rm -rf cmake/DumpScannedXMP/build
fi
if [ -e cmake/DumpScannedXMP/build_x64 ]
then
rm -rf cmake/DumpScannedXMP/build_x64
fi
if [ -e cmake/ModifyingXMP/build ]
then
rm -rf cmake/ModifyingXMP/build
fi
if [ -e cmake/ModifyingXMP/build_x64 ]
then
rm -rf cmake/ModifyingXMP/build_x64
fi
if [ -e cmake/ReadingXMP/build ]
then
rm -rf cmake/ReadingXMP/build
fi
if [ -e cmake/ReadingXMP/build_x64 ]
then
rm -rf cmake/ReadingXMP/build_x64
fi
if [ -e cmake/XMPCommand/build ]
then
rm -rf cmake/XMPCommand/build
fi
if [ -e cmake/XMPCommand/build_x64 ]
then
rm -rf cmake/XMPCommand/build
fi
if [ cmake/XMPCoreCoverage/build_x64 ]
then
rm -rf cmake/XMPCommand/build_x64
fi
if [ -e cmake/XMPCoreCoverage/build ]
then
rm -rf cmake/XMPCoreCoverage/build
fi
if [ -e cmake/XMPFilesCoverage/build_x64 ]
then
rm -rf cmake/XMPCoreCoverage/build_x64
fi
if [ -e cmake/XMPFilesCoverage/build ]
then
rm -rf cmake/XMPFilesCoverage/build
fi
if [ -e cmake/XMPIterations/build_x64 ]
then
rm -rf cmake/XMPFilesCoverage/build_x64
fi
if [ -e cmake/XMPIterations/build ]
then
rm -rf cmake/XMPIterations/build
fi
if [ -e cmake/XMPIterations/build_x64 ]
then
rm -rf cmake/XMPIterations/build_x64
fi
if [ -e xcode ]
then
rm -rf xcode
fi
if [ -e ../target/macintosh ]
then
rm -rf ../target/macintosh
fi
echo "Done"
exit 0;
}
Xcode32()
{
#create 32bit Xcode Project
DIR="xcode/intel"
TOOLCHAIN="$scriptdir/../../build/shared/ToolchainLLVM.cmake"
bit64="0"
}
Xcode64()
{
#create 64bit Xcode Project
DIR="xcode/intel_64"
TOOLCHAIN="$scriptdir/../../build/shared/ToolchainLLVM.cmake"
bit64="1"
}

createProject()
{
#creating the directory
if [ ! -e $DIR ]
then
if ! mkdir -p $DIR
then
echo "ERROR: Cannot create $DIR"
exit 1
fi
fi
if ! cd $DIR
then
echo "ERROR: Cannot change to $DIR"
exit 1
fi

#generate the make file
echo "PWD is $PWD, CMAKE LOCATION IS $CMAKE, dir is $DIR"
if ! ("$CMAKE" -D CMAKE_CL_64=$bit64  -G "Xcode" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"  ../../cmake)
then
echo "ERROR: CMAKE tool failed"
exit 1
else
echo "Xcode project created successfully"
fi
}

echo "Enter your choice:"
echo "1. Clean"
echo "2. 32 Bit Xcode Project"
echo "3. 64 Bit Xcode Project"
echo "4. Build All"


read choice

case $choice in
  1) clean; break;;
  2) Xcode32; createProject;;
  3) Xcode64; createProject;;
  4) Xcode32; createProject; cd -; Xcode64; createProject;;
  *) echo "ERROR: Invalid Choice, Exiting"; exit 1;;
esac


