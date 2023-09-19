@echo off
set CCF=-g -Og -DALLOCATOR_DEBUG -DDEBUG -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF= 
make %*
