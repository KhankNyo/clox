CC?=gcc
CCF?=-Ofast -flto -std=c99 -Wall -Wextra -Wpedantic -DOBJSTR_FLEXIBLE_ARR
LDF?=-flto
LIBS=


ifeq ($(OS),Windows_NT)
	EXEC_FMT=.exe
else
	EXEC_FMT=
endif

SRCS=$(wildcard src/*.c)
OBJS=$(patsubst src/%.c,obj/%.o,$(SRCS))
OUTPUT=bin/Lox$(EXEC_FMT)



.PHONY:all clean


all:$(OUTPUT)



obj bin:
	mkdir $@

$(OUTPUT):obj bin $(OBJS)
	$(CC) $(LDF) -o $@ $(OBJS) $(LIBS)

obj/%.o:src/%.c
	$(CC) $(CCF) -c $< -o $@


clean:
	rm -f obj/* bin/*
	rmdir obj bin



