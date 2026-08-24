/*
 * Lexical analyzer for COOL.
 */

%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

#define yylval cool_yylval
#define yylex  cool_yylex

#define MAX_STR_CONST 1025
#define YY_NO_UNPUT

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  if ((result = fread((char *)buf, sizeof(char), max_size, fin)) < 0) \
    YY_FATAL_ERROR("read() in flex scanner failed");

extern FILE *fin;
extern int curr_lineno;
extern YYSTYPE cool_yylval;

char string_buf[MAX_STR_CONST];
char *string_buf_ptr;
int comment_depth = 0;

#define STRING_FULL() (string_buf_ptr - string_buf >= MAX_STR_CONST - 1)

#define APPEND_CHAR(c)                     \
  do {                                     \
    if (STRING_FULL()) {                   \
      cool_yylval.error_msg =              \
          (char *)"String constant too long"; \
      BEGIN(STRING_ERR);                   \
      return ERROR;                        \
    }                                      \
    *string_buf_ptr++ = (c);               \
  } while (0)

extern "C" int yywrap(void) { return 1; }
%}

%x COMMENT
%x STRING
%x STRING_ERR

DIGIT       [0-9]
LETTER      [A-Za-z]
WHITESPACE  [ \t\r\v\f]+
NEWLINE     \n
DARROW      =>

%%

{DARROW}                { return DARROW; }
"<-"                    { return ASSIGN; }
"<="                    { return LE; }

(?i:class)              { return CLASS; }
(?i:else)               { return ELSE; }
(?i:if)                 { return IF; }
(?i:fi)                 { return FI; }
(?i:in)                 { return IN; }
(?i:inherits)           { return INHERITS; }
(?i:let)                { return LET; }
(?i:loop)               { return LOOP; }
(?i:pool)               { return POOL; }
(?i:then)               { return THEN; }
(?i:while)              { return WHILE; }
(?i:case)               { return CASE; }
(?i:esac)               { return ESAC; }
(?i:new)                { return NEW; }
(?i:isvoid)             { return ISVOID; }
(?i:not)                { return NOT; }
(?i:of)                 { return OF; }

t(?i:rue)               { cool_yylval.boolean = true;  return BOOL_CONST; }
f(?i:alse)              { cool_yylval.boolean = false; return BOOL_CONST; }

"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"<"                     { return '<'; }
"="                     { return '='; }
"."                     { return '.'; }
";"                     { return ';'; }
"~"                     { return '~'; }
":"                     { return ':'; }
"@"                     { return '@'; }
","                     { return ','; }
"("                     { return '('; }
")"                     { return ')'; }
"{"                     { return '{'; }
"}"                     { return '}'; }

{DIGIT}+                {
                          cool_yylval.symbol = inttable.add_string(yytext);
                          return INT_CONST;
                        }

[A-Z]({LETTER}|{DIGIT}|_)* {
                          cool_yylval.symbol = idtable.add_string(yytext);
                          return TYPEID;
                        }

[a-z]({LETTER}|{DIGIT}|_)* {
                          cool_yylval.symbol = idtable.add_string(yytext);
                          return OBJECTID;
                        }

{NEWLINE}               { curr_lineno++; }
{WHITESPACE}            { }

"--".*                  { }

"(*"                    { comment_depth = 1; BEGIN(COMMENT); }
"*)"                    {
                          cool_yylval.error_msg = "Unmatched *)";
                          return ERROR;
                        }

<COMMENT>"(*"           { comment_depth++; }
<COMMENT>"*)"           {
                          if (--comment_depth == 0)
                            BEGIN(INITIAL);
                        }
<COMMENT>{NEWLINE}      { curr_lineno++; }
<COMMENT>.              { }
<COMMENT><<EOF>>        {
                          BEGIN(INITIAL);
                          cool_yylval.error_msg = "EOF in comment";
                          return ERROR;
                        }

"\""                    { string_buf_ptr = string_buf; BEGIN(STRING); }

<STRING>"\""            {
                          BEGIN(INITIAL);
                          *string_buf_ptr = '\0';
                          cool_yylval.symbol = stringtable.add_string(string_buf);
                          return STR_CONST;
                        }
<STRING>{NEWLINE}       {
                          BEGIN(INITIAL);
                          curr_lineno++;
                          cool_yylval.error_msg = "Unterminated string constant";
                          return ERROR;
                        }
<STRING>\\n             { APPEND_CHAR('\n'); }
<STRING>\\t             { APPEND_CHAR('\t'); }
<STRING>\\b             { APPEND_CHAR('\b'); }
<STRING>\\f             { APPEND_CHAR('\f'); }
<STRING>\\\n            { APPEND_CHAR('\n'); }
<STRING>\0|\\\0         {
                          cool_yylval.error_msg =
                              (char *)"String contains null character";
                          BEGIN(STRING_ERR);
                          return ERROR;
                        }
<STRING>\\.             { APPEND_CHAR(yytext[1]); }
<STRING>.               { APPEND_CHAR(yytext[0]); }
<STRING><<EOF>>         {
                          BEGIN(INITIAL);
                          cool_yylval.error_msg = "EOF in string constant";
                          return ERROR;
                        }

<STRING_ERR>"\""          { BEGIN(INITIAL); }
<STRING_ERR>{NEWLINE}   { curr_lineno++; BEGIN(INITIAL); }
<STRING_ERR>\\\n        { curr_lineno++; }
<STRING_ERR>.           { }

.                       {
                          cool_yylval.error_msg = yytext;
                          return ERROR;
                        }

%%
