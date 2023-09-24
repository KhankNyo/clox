@echo off
set CCF=-g -Og -DDEBUG -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF= 
make %*
