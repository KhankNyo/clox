@echo off
set CCF=-Ofast -flto -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF=-flto
make %*
