@echo off
set CCF=-flto -Ofast -DDEBUG -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
set LDF=-flto
make %*
