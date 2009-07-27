#include <common.h>

#include "net-snmp-config.h"
#include "net-snmp-includes.h"

#ifndef _RATEDSNMP_H_
#define _RATEDSNMP_H_ 1

/* Target state */
enum targetState {NEW, LIVE, STALE};

typedef struct target_struct {
    char *objoid;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    char *table;
    unsigned short bits;
    unsigned int iid;
    enum targetState init;
    unsigned long long last_value;
    struct timeval last_time;
    struct target_struct *next;
} target_t;

typedef struct host_struct {
    char *host;
    struct snmp_session session;
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
int free_target_list(host_t *);

#endif /* not _RATEDSNMP_H_ */
