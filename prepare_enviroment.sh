#!/bin/bash

sudo apt update
sudo apt upgrade

sudo apt install nasm make qemu qemu-system-i386

sudo apt install wget bochs bochs-sdl bochsbios vgabios

mkdir Downloads
cd Downloads

wget https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x64
sudo chmod +x ./open-watcom-2_0-c-linux-x64
sudo ./open-watcom-2_0-c-linux-x64
rm ./open-watcom-2_0-c-linux-x64
rm -fr ./Downloads

sudo apt install gcc
