@echo off
set CC=gcc
set CCF= -O0 -g -DBLAZINGLY_FAST -DALLOCATOR_DEFAULT -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic
set LDF= 
make %*
