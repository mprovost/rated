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

static template_t *template_tail;
static template_t template_dummy;

char *community;
size_t community_len;

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

/* community */
%token T_COMM

/* host */
%token T_HOST

/* per-host */
%token HST_ADDR HST_SVER 

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
    template_tail = &template_dummy;
    template_dummy.next = NULL;
}
'{' template_directives '}'
{
    /* we don't store the template name so free it to avoid a memory leak */
    free($2);
};

template_directives: template_directives target_directive
                   | template_directives community_entry
                   | target_directive
                   | host_entry
                   ;

target_directive: TMPL_TRGT L_OID
{
    template_t *ttemplate;
    ttemplate = calloc(1, sizeof(template_t));
    ttemplate->next = NULL;
    ttemplate->objoid = $2;
    ttemplate->anOID_len = MAX_OID_LEN;

    /* generate an internal oid from the string */
    if (snmp_parse_oid($2, ttemplate->anOID, &ttemplate->anOID_len)) {
        targets++;
        template_tail->next = ttemplate;
        template_tail = template_tail->next;
    } else {
        fprintf(stderr, "Couldn't parse target oid \"%s\" at line %d:\n", $2, yylineno);
        snmp_perror($2);
        free(ttemplate);
    }
};

community_entry: T_COMM L_IDENT
{
    community = $2;
    community_len = strlen($2);
}
'{' community_directives '}';

community_directives: host_entry
                    | community_directives host_entry
                    ;

host_entry:   T_HOST L_IDENT
{
    thst = calloc(1, sizeof(host_t));
    thst->host = $2;
    thst->next = NULL;
    /* save pointer to enclosing template */
    thst->template = template_dummy.next;
    /* set up the snmp session */
    snmp_sess_init(&thst->session);
    /* TODO this is deprecated append to the peername */
    thst->session.remote_port = set->snmp_port;
    thst->session.community = community;
    thst->session.community_len = community_len;
    /* create the first target */
    thst->targets = calloc(1, sizeof(target_t));
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
              | sver_directive
              ;

addr_directive : HST_ADDR L_IPADDR
{
    thst->address = $2;
    thst->session.peername = $2;
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
