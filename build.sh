#!/bin/sh
mkdir -p build/dist
gcc -Wall -Wextra -g -I. -Iinclude -o build/dist/beacon-vm src/beacon-vm/UnityBuild.c
