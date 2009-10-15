#include <common.h>

#include "net-snmp-config.h"
#include "net-snmp-includes.h"

#ifndef _RATEDSNMP_H_
#define _RATEDSNMP_H_ 1

typedef struct getnext_struct {
    char *objoid;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    unsigned long iid;
    unsigned long long last_value;
    struct timeval last_time;
    struct getnext_struct *next;
} getnext_t;

typedef struct target_struct {
    char *objoid;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    struct getnext_struct *getnexts;
    struct getnext_struct *current;
    struct target_struct *next;
} target_t;

typedef struct host_struct {
    char *host;
    char *host_esc;
    char *address;
    netsnmp_session session;
    target_t *targets;
    target_t *current;
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
void print_targets(target_t *);
int free_target_list(host_t *);
target_t *copy_target_list(target_t *);

#endif /* not _RATEDSNMP_H_ */
