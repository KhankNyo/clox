@echo off
set CC=gcc
set CCF=-DALLOCATOR_DEFAULT -DDEBUG_PRINT_CODE -Ofast -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic
set LDF= 
make %*
