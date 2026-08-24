ASSN     = 2
CLASSDIR = /usr/class

SRC      = cool.flex lextest.cc utilities.cc stringtab.cc handle_flags.cc
GEN      = cool-lex.cc
OBJS     = lextest.o utilities.o stringtab.o handle_flags.o cool-lex.o

CPPINCLUDE = -I. -I$(CLASSDIR)/include/PA$(ASSN) -I$(CLASSDIR)/src/PA$(ASSN)

CC       = g++
CFLAGS   = -g -Wall -Wno-unused -Wno-write-strings $(CPPINCLUDE)
FLEX     = flex -d -ocool-lex.cc
LIB      = -lfl

.PHONY: all clean test

all: lexer

lexer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $@

cool-lex.cc: cool.flex
	$(FLEX) $<

cool-lex.o: cool-lex.cc
	$(CC) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) -c $<

test: lexer
	./run_tests.sh

clean:
	rm -f $(OBJS) lexer $(GEN) test.output *.d
