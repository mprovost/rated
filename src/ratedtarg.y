%{
#include "common.h"
#include "rated.h"
#include "ratedsnmp.h"

int yyerror(const char *s);
int yylex(void);
extern int yylineno, lineno;
extern char *yytext;

extern unsigned int hosts;
extern unsigned int targets;
extern host_t *hosts_tail;
extern config_t *set;

static host_t *thst;

static target_t *target_tail;
static target_t target_dummy;

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
%token T_TMPL

/* per-template */
%token TMPL_TRGT

/* host */
%token T_HOST

/* per-host */
%token HST_ADDR HST_COMM HST_SVER 

%%

config                :
              | statements
               ;

statements    : statements statement
              | statement
              ;

statement     : template_entry
              | error ';'
                      { fprintf(stderr, "';' line %d\n", yylineno); yyerrok; }
              | error '}'
                      { fprintf(stderr, "'}' line %d\n", yylineno); yyerrok; }
              ;

template_entry: T_TMPL L_IDENT
{
    target_tail = &target_dummy;
    target_dummy.next = NULL;
}
'{' template_directives '}'
{
    free_target_list(target_dummy.next);
    /* we don't store the template name so free it to avoid a memory leak */
    free($2);
};

template_directives: template_directives target_directive
                   | template_directives host_entry
                   | target_directive
                   | host_entry
                   ;

target_directive: TMPL_TRGT L_OID
{
    target_t *ttgt;
    ttgt = calloc(1, sizeof(target_t));
    ttgt->next = NULL;
    ttgt->objoid = $2;
    ttgt->anOID_len = MAX_OID_LEN;

    /* generate an internal oid from the string */
    if (snmp_parse_oid($2, ttgt->anOID, &ttgt->anOID_len)) {
        targets++;
        target_tail->next = ttgt;
        target_tail = target_tail->next;
    } else {
        fprintf(stderr, "Couldn't parse target oid \"%s\" at line %d:\n", $2, yylineno);
        snmp_perror($2);
        free(ttgt);
    }
};

host_entry:   T_HOST L_IDENT
{
    thst = malloc(sizeof(host_t));
    bzero(thst, sizeof(host_t));
    thst->host = $2;
    thst->next = NULL;
    /* copy the target list from the template */
    thst->targets = copy_target_list(target_dummy.next);
    thst->current = thst->targets;
    /* set up the snmp session */
    snmp_sess_init(&thst->session);
    /* TODO this is deprecated append to the peername */
    thst->session.remote_port = set->snmp_port;
}
'{' host_directives '}'
{
    hosts++;
    hosts_tail->next = thst;
    hosts_tail = hosts_tail->next;
};

host_directives       : host_directives host_directive
              | host_directive
              ;

host_directive        : addr_directive
              | comm_directive
              | sver_directive
              ;

addr_directive : HST_ADDR L_IPADDR
{
    thst->address = $2;
    thst->session.peername = $2;
};

comm_directive        : HST_COMM L_IDENT
{
      thst->session.community = $2;
      thst->session.community_len = strlen($2);
};

sver_directive        : HST_SVER L_NUMBER
{
      if ((unsigned short)$2 == 2)
        thst->session.version = SNMP_VERSION_2c;
      else
        thst->session.version = SNMP_VERSION_1;
};


%%
int yyerror(const char *msg)
{

      fprintf(stderr, "targets failure on line %d - %s\n", lineno, msg);
      fprintf(stderr, "last token parsed: %s\n", yytext);

      exit(1);
}
