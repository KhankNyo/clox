@echo off
set CCF=-DDEBUG -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF= 
make %*
