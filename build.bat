@echo off
set CC=gcc
set CCF=-Ofast -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF= 
make %*
