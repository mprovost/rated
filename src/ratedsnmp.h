#include <common.h>

#include "net-snmp/net-snmp-config.h"
#include "net-snmp/net-snmp-includes.h"

#ifndef _RATEDSNMP_H_
#define _RATEDSNMP_H_ 1

/* internal poll table */
#define RATED "rated"
/* oid lookup table */
#define OIDS "oids"

typedef struct getnext_struct {
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    unsigned long iid;
    unsigned long long last_value;
    struct timeval last_time;
    struct getnext_struct *next;
} getnext_t;

typedef struct target_struct {
    unsigned long long getnext_counter;
    struct getnext_struct *getnexts;
    /* storage for the internal polling data */
    struct getnext_struct poll;
    struct target_struct *next;
} target_t;

typedef struct template_struct {
    char *objoid;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    struct template_struct *next;
} template_t;

typedef struct host_struct {
    char *host;
    char *host_esc;
    char *address;
    struct template_struct *template;
    unsigned long sysuptime;
    netsnmp_session session;
    target_t *targets;
    struct host_struct *next;
} host_t;

typedef struct crew_struct {
    int running;
    worker_t member[MAX_THREADS];
    pthread_mutex_t mutex;
    pthread_cond_t done;
    pthread_cond_t go;
    host_t *current;
} crew_t;

/* Precasts: ratedhash.c */
host_t *hash_target_file(char *);
void print_template(template_t *);
int free_template_list(template_t *);
int free_host_list(host_t *);
int free_target_list(target_t *);
int free_getnext_list(getnext_t *);

#endif /* not _RATEDSNMP_H_ */
