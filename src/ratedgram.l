%option yylineno
%option noyywrap
%{
#include <stdio.h>
#include <string.h>
#include "rated.h"
#include "ratedtarg.h"


unsigned int lineno = 1;
int yylex(void);
%}

token         [{};]
endtoken      ;$
newline       [\n]*
whitespace    [ \t\v\f]+
hashcmnt      #.*$
cppcmnt       "//".*$
ipv4addr      ([0-9]{1,3}\.){1,3}[0-9]{1,3}
oid           (\.[0-9]+)+
string        \"[^\"\n]+\"
number        [0-9]+
identifier    [a-zA-Z0-9]([^; \t]|[:print:])*
true          [Tt]rue|[Yy]es
false         [Ff]alse|[Nn]o

%%
{newline}     { ++lineno; }
{whitespace}  { }
{hashcmnt}    { }
{cppcmnt}     { }
{endtoken}    { }

template      { return T_TMPL; }
community     { return T_COMM; }
snmpver       { return T_SVER; }
target        { return TMPL_TRGT; }

host          { return T_HOST; }
address       { return HST_ADDR; }

{token}       { return yytext[0]; }
{number}      {
#ifdef HAVE_STRTOULL
                yylval.number = strtoull(yytext, NULL, 0);
#else
                yylval.number = strtoul(yytext, NULL, 0);
#endif
                return L_NUMBER;
              }
{ipv4addr}    { yylval.string = strdup(yytext); return L_IPADDR; }
{oid}         { yylval.string = strdup(yytext); return L_OID; }
{string}      { yylval.string = strdup(yytext+1);
                  if(yylval.string[yyleng-2] != '"')
                      fprintf(stderr,"Unterminated character string\n");
                  else
                      yylval.string[yyleng-2] = '\0'; /* remove close quote */
                return L_STRING;
              }
{true}        { yylval.boolean = TRUE; return L_BOOLEAN; }
{false}       { yylval.boolean = FALSE; return L_BOOLEAN; }
{identifier}  { yylval.string = strdup(yytext); return L_IDENT; }

			

.             { fprintf(stderr,"unexpected character: %c", yytext[0]); }
%%
