/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

int comment_depth = 0; /*for nested comments*/ 


extern "C" int yywrap(void) { return 1; }

%}

/*State definitions for strings/comments*/
%x COMMENT
%x STRING

/*Other definitions*/
DIGIT           [0-9]
WHITESPACE      [ \t\r\v\f]+
NEWLINE         \n
LETTER		[A-Za-z]
DARROW          =>

%%
  /*
  *  The multiple-character operators.
  */
{DARROW}                { return (DARROW); }
"<-"                    { return (ASSIGN); }
"<="                    { return (LE); }

  /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
(?i:class)              { return (CLASS); }
(?i:else)               { return (ELSE); }
(?i:if)                 { return (IF); }
(?i:fi)                 { return (FI); }
(?i:in)                 { return (IN); }
(?i:inherits)           { return (INHERITS); }
(?i:let)                { return (LET); }
(?i:loop)               { return (LOOP); }
(?i:pool)               { return (POOL); }
(?i:then)               { return (THEN); }
(?i:while)              { return (WHILE); }
(?i:case)               { return (CASE); }
(?i:esac)               { return (ESAC); }
(?i:new)                { return (NEW); }
(?i:isvoid)             { return (ISVOID); }
(?i:not)                { return (NOT); }

 /* Boolean Constants */
t(?i:rue)               { cool_yylval.boolean = true; return (BOOL_CONST); }
f(?i:alse)              { cool_yylval.boolean = false; return (BOOL_CONST); }

 /*
  * Integers and Identifiers
  */
{DIGIT}+                {
                          cool_yylval.symbol = inttable.add_string(yytext);
                          return (INT_CONST);
                        }

 /* Type Identifiers (Starts with Uppercase letter) */
[A-Z]({LETTER}|{DIGIT}|_)*    {
                                cool_yylval.symbol = idtable.add_string(yytext);
                                return (TYPEID);
                              }

 /* Object Identifiers (Starts with Lowercase letter) */
[a-z]({LETTER}|{DIGIT}|_)*    {
                                cool_yylval.symbol = idtable.add_string(yytext);
                                return (OBJECTID);
                              }

 /*
  * Single-character operators/tokens
  */
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

 /*
  * Whitespace & Tracking newlines
  */
{NEWLINE}               { curr_lineno++; }
{WHITESPACE}            { /* Ignore whitespace spacing rules */ }


 /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ comments and strings ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

"--".*                  { /* Single line comment: ignore until newline */ }

"(*"                    { 
                          comment_depth = 1; 
                          BEGIN(COMMENT); 
                        }
"*)"                    { 
                          cool_yylval.error_msg = "Unmatched *)"; 
                          return (ERROR); 
                        }

<COMMENT>"(*"           { comment_depth++; }
<COMMENT>"*)"           { 
                          comment_depth--; 
                          if (comment_depth == 0) { BEGIN(INITIAL); } 
                        }
<COMMENT>{NEWLINE}      { curr_lineno++; }
<COMMENT>.              { /* Ignore characters inside comments */ }
<COMMENT><<EOF>>        { 
                          BEGIN(INITIAL); 
                          cool_yylval.error_msg = "EOF in comment"; 
                          return (ERROR); 
                        }

 /*
  *  String constants (C syntax)
  *  So basically<STRING> is a state with rules that apply. When a quote is encountered, it starts the string state, exiting intiial state 
  *  where it typically checks for keywords, identifiers, etc. Then when a quote is encountered in string mode <STRING>, the lexer mode is set back to initial.
  *
  *  Same applies to the <COMMENT> state above. 
  *
  *  For the case of single line comments: --.* matches every possible scenario so no state change is needed. This is because . in regex is any non-newline character, so the whole thing is one token
  *
  */
"\""                    { 
                          string_buf_ptr = string_buf; 
                          BEGIN(STRING); 
                        }

<STRING>"\""            { 
                          BEGIN(INITIAL);
                          *string_buf_ptr = '\0';
                          cool_yylval.symbol = stringtable.add_string(string_buf);
                          return (STR_CONST);
                        }

<STRING>{NEWLINE}       { 
                          BEGIN(INITIAL);
                          curr_lineno++;
                          cool_yylval.error_msg = "Unterminated string constant";
                          return (ERROR);
                        }

<STRING>\\[0-7]{1,3}    { /* Handle octal escapes if needed, or parse standard character escapes */ }

<STRING>\\n             { 
                          if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                              BEGIN(INITIAL); cool_yylval.error_msg = "String constant too long"; return (ERROR);
                          }
                          *string_buf_ptr++ = '\n'; 
                        }
<STRING>\\t             { *string_buf_ptr++ = '\t'; }
<STRING>\\b             { *string_buf_ptr++ = '\b'; }
<STRING>\\f             { *string_buf_ptr++ = '\f'; }

<STRING>\\\n            { curr_lineno++; *string_buf_ptr++ = '\n'; }
<STRING>\\.             { *string_buf_ptr++ = yytext[1]; }

<STRING>\0              {
                          /* Cool strings cannot contain embedded null characters */
                          cool_yylval.error_msg = "String contains null character";
                          /* Advance past rest of string to recover */
                          while(yyinput() != '"' && yyinput() != '\n' && yyinput() != EOF);
                          BEGIN(INITIAL);
                          return (ERROR);
                        }

<STRING>.               {
                          if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                              BEGIN(INITIAL); 
                              cool_yylval.error_msg = "String constant too long"; 
                              return (ERROR);
                          }
                          *string_buf_ptr++ = yytext[0];
                        }

<STRING><<EOF>>       {
                          BEGIN(INITIAL);
                          cool_yylval.error_msg = "EOF in string constant";
                          return (ERROR);
                        }

%%
