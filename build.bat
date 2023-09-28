@echo off
set CC=gcc
set CCF=-g -Og -DDEBUG -DBLAZINGLY_FAST -DALLOCATOR_DEFAULT -DOBJSTR_FLEXIBLE_ARR -Wall -Wextra -Wpedantic
set LDF= 
make %*
