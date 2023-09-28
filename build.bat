@echo off
set CC=gcc
set CCF=-Ofast -DBLAZINGLY_FAST -DALLOCATOR_DEFAULT -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic
set LDF= 
make %*
