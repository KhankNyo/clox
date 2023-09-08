@echo off
set CCF=-flto -Ofast -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF=-flto
make %*
