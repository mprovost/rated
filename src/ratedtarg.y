%{
#include "common.h"
#include "rated.h"

int yyerror(const char *s);
int yylex(void);
extern int yylineno, lineno;
extern char *yytext;

extern unsigned int hosts;
extern unsigned int targets;
extern host_t *hosts_tail;

static host_t *thst;
target_t *ttgt;
target_t *targets_tail;
target_t target_dummy;

#define YYDEBUG 1
%}

%union {
      char *string;
      unsigned long long number;
      int boolean;
}

%token <string>               L_STRING
%token <string>               L_IDENT
%token <string>               L_IPADDR
%token <string>               L_OID
%token <number>               L_NUMBER
%token <boolean>              L_BOOLEAN

/* top level */
%token T_HOST

/* per-host */
%token HST_COMM HST_SVER HST_TRGT

/* per-target */
%token TGT_BITS TGT_TBL TGT_ID TGT_SPEED TGT_DESCR

%%

config                :
              | statements
               ;

statements    : statements statement
              | statement
              ;

statement     : host_entry
              | error ';'
                      { fprintf(stderr, "';' line %d\n", yylineno); yyerrok; }
              | error '}'
                      { fprintf(stderr, "'}' line %d\n", yylineno); yyerrok; }
              ;

host_entry:   T_HOST L_IPADDR
{
      thst = malloc(sizeof(host_t));
      bzero(thst, sizeof(host_t));
      thst->host = $2;
      thst->next = NULL;
      target_dummy.next = NULL;
      thst->targets = &target_dummy;
}
'{' host_directives '}'
{
    hosts++;
    thst->targets = target_dummy.next;
    thst->current = thst->targets;
    hosts_tail->next = thst;
    hosts_tail = hosts_tail->next;
};

host_directives       : host_directives host_directive
              | host_directive
              ;

host_directive        : comm_directive
              | sver_directive
              | target_entry
              ;

comm_directive        : HST_COMM L_IDENT
{
      thst->community = $2;
};

sver_directive        : HST_SVER L_NUMBER
{
      thst->snmp_ver = (unsigned short)$2;
};

target_entry  : HST_TRGT L_OID
{
      ttgt = malloc(sizeof(target_t));
      bzero(ttgt, sizeof(target_t));
      ttgt->objoid = $2;
      ttgt->init = NEW;
      ttgt->next = NULL;
}
'{' tgt_directives '}'
{
    targets++;
    thst->targets->next = ttgt;
    thst->targets = thst->targets->next;
};
tgt_directives        : tgt_directives tgt_directive
              | tgt_directive
              ;

tgt_directive : bits_directive
              | table_directive
              | id_directive
              ;

bits_directive        : TGT_BITS L_NUMBER
{
      ttgt->bits = $2;
};

table_directive       : TGT_TBL L_IDENT
{
      ttgt->table = $2;
};

id_directive  : TGT_ID L_NUMBER
{
      ttgt->iid = $2;
};

%%
int yyerror(const char *msg)
{

      fprintf(stderr, "targets failure on line %d - %s\n", lineno, msg);
      fprintf(stderr, "last token parsed: %s\n", yytext);

      exit(1);
}
