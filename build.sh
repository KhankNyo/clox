#!/bin/sh


export CC=gcc
# nan boxing slower on linux??? 
export CCF="-Ofast -flto -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic"
export LDF="-flto"

make "$@"

