#!/bin/bash


set -xe

nasm -f elf64 test.s -o test.o
ld test.o -o out

rm test.o

