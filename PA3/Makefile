ASSN     = 3
CLASSDIR = /usr/class
LEXER    = ../PA2/lexer

SRC      = cool.y cool-tree.handcode.h
CSRC     = parser-phase.cc utilities.cc stringtab.cc dumptype.cc \
           tree.cc cool-tree.cc tokens-lex.cc handle_flags.cc
GEN      = cool-parse.cc cool.tab.h
OBJS     = parser-phase.o utilities.o stringtab.o dumptype.o tree.o \
           cool-tree.o tokens-lex.o handle_flags.o cool-parse.o

CPPINCLUDE = -I. -I$(CLASSDIR)/include/PA$(ASSN) -I$(CLASSDIR)/src/PA$(ASSN)
BFLAGS   = -d -v -y -b cool --debug -p cool_yy

CC       = g++
CFLAGS   = -g -Wall -Wno-unused -Wno-deprecated -Wno-write-strings -DDEBUG $(CPPINCLUDE)
LIB      = -lfl

.PHONY: all clean test lexer

all: parser

lexer:
	$(MAKE) -C ../PA2 lexer

parser: lexer $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $@

cool-parse.cc cool.tab.h: cool.y
	bison $(BFLAGS) cool.y
	mv -f cool.tab.c cool-parse.cc

cool-parse.o: cool-parse.cc cool.tab.h
	$(CC) $(CFLAGS) -c cool-parse.cc

%.o: %.cc
	$(CC) $(CFLAGS) -c $<

test: parser lexer
	./run_tests.sh

clean:
	rm -f $(OBJS) parser $(GEN) good.output bad.output *.d cool.output
