%{
#include "common.h"
#include "rated.h"
#include "ratedsnmp.h"
#include "rateddbi.h"

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
static template_t *parent;
static template_t *ttemplate;

u_char *community;
size_t community_len;
long sver;

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

/* snmp version */
%token T_SVER

/* host */
%token T_HOST

/* per-host */
%token HST_ADDR

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
    parent = template_dummy.next;
    template_tail = &template_dummy;
    template_dummy.next = NULL;
}
'{' template_directives '}'
{
    /* we don't store the template name so free it to avoid a memory leak */
    free($2);
    /* the last target gets set to the head of the parent template */
    ttemplate->next = parent;
    /* reset the parent */
    template_dummy.next = parent;
};

template_directives: template_entry
                   | community_entry
                   | target_directive
                   | template_directives template_entry
                   | template_directives community_entry
                   | template_directives target_directive
                   ;

target_directive: TMPL_TRGT L_OID
{
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
    community = (u_char *)$2;
    community_len = strlen($2);
}
'{' community_directives '}';

community_directives: sver_entry
                    | community_directives sver_entry
                    ;

sver_entry : T_SVER L_NUMBER
{
      if ((unsigned short)$2 == 2)
        sver = SNMP_VERSION_2c;
      else
        sver = SNMP_VERSION_1;
}
'{' sver_directives '}';

sver_directives: host_entry
               | sver_directives host_entry
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
    thst->session.version = sver;
    /* create the first target */
    thst->targets = calloc(1, sizeof(target_t));
    /* check and create a db table */
    if (set->dbon) {
        thst->host_esc = db_check_and_create_data_table(thst->host);
        debug(DEBUG, "host: %s -> host_esc: %s\n", thst->host, thst->host_esc);
    }
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
              ;

addr_directive : HST_ADDR L_IPADDR
{
    thst->address = $2;
    thst->session.peername = $2;
};


%%
int yyerror(const char *msg)
{

      fprintf(stderr, "targets failure on line %d - %s\n", lineno, msg);
      fprintf(stderr, "last token parsed: %s\n", yytext);

      exit(1);
}
