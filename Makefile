CC=gcc
CCF=-Wall -Wextra -Wpedantic -O0 -DDEBUG
LDF=
LIBS=


ifeq ($(OS),Windows_NT)
	EXEC_FMT=.exe
else
	EXEC_FMT=
endif

SRCS=$(wildcard src/*)
OBJS=$(patsubst src/%.c,obj/%.o,$(SRCS))
OUTPUT=bin/main$(EXEC_FMT)



.PHONY:all clean


all:$(OUTPUT)


obj bin:
	mkdir $@

$(OUTPUT):obj bin $(OBJS)
	$(CC) $(LDF) -o $@ $(OBJS) $(LIBS)

obj/%.o:src/%.c
	$(CC) $(CCF) -c $^ -o $@ 


clean:
	rm -f $(OBJS) $(OUTPUT)
	rmdir obj bin



