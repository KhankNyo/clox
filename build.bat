@echo off
set CC=gcc
set CCF=-DALLOCATOR_DEFAULT -Ofast -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic
set LDF= 
make %*
