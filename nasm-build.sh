#!/bin/bash


set -xe

nasm -f elf64 $1 -o tmp.o
ld tmp.o -o a.out
rm tmp.o

