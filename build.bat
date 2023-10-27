@echo off
set CC=gcc
set CCF= -Ofast -DDEBUG_PRINT_CODE -DBLAZINGLY_FAST -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic
set LDF= 
make %*
