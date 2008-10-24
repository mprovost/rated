/****************************************************************************
   Program:     $Id: rtg.h,v 1.43 2008/01/19 03:01:32 btoneill Exp $ 
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG headers
****************************************************************************/

#include <common.h>
#include <sys/param.h>

#ifndef _RTG_H_
#define _RTG_H_ 1

/* Defines */ 
#ifndef FALSE
# define FALSE 0
#endif
#ifndef TRUE
# define TRUE !FALSE
#endif

/* Define Linux pthread brokeness */
/* #define BROKEN_LINUX 1 */

/* Constants */
#define MAX_THREADS 100
#define BUFSIZE 512
#define BITSINBYTE 8
#define THIRTYTWO 4294967295ul
#ifdef HAVE_STRTOLL
# define SIXTYFOUR 18446744073709551615ull
#else
# define SIXTYFOUR 18446744073709551615ul
#endif
#define KILO 1000
#define MEGA (unsigned int)(KILO * KILO)
#define GIGA (unsigned long long)(MEGA * KILO)
#define TERA (unsigned long long)(GIGA * KILO)


#define POLLS_PER_TRANSACTION 100

/* Define CONFIG_PATHS places to search for the rtg.conf file.  Note
   that RTG_HOME, as determined during autoconf is one path */
#define CONFIG_PATHS 3
#define CONFIG_PATH_1 ""
#define CONFIG_PATH_2 "/etc/"

/* Defaults */
#define DEFAULT_CONF_FILE "rtg.conf"
#define DEFAULT_THREADS 5
#define DEFAULT_INTERVAL 300
#define DEFAULT_HIGHSKEWSLOP 3
#define DEFAULT_LOWSKEWSLOP .5
#define DEFAULT_OUT_OF_RANGE 93750000000ull
#define DEFAULT_DB_DRIVER "librtgmysql.so"
#define DEFAULT_DB_HOST "localhost"
#define DEFAULT_DB_DB "rtg"
#define DEFAULT_DB_USER "snmp"
#define DEFAULT_DB_PASS "rtgdefault"
#define DEFAULT_SNMP_VER 1
#define DEFAULT_SNMP_PORT 161
#define DEFAULT_SYSLOG_FACILITY LOG_LOCAL2

/* PID File */
#define PIDFILE "/var/run/rtgpoll.pid"

#define STAT_DESCRIP_ERROR 99
#define HASHSIZE 5000

/* pthread error messages */
#define PML_ERR "pthread_mutex_lock error\n"
#define PMU_ERR "pthread_mutex_unlock error\n"
#define PCW_ERR "pthread_cond_wait error\n"
#define PCB_ERR "pthread_cond_broadcast error\n"

/* pthread macros */
#define PT_MUTEX_LOCK(x) if (pthread_mutex_lock(x) != 0) printf(PML_ERR)
#define PT_MUTEX_UNLOCK(x) if (pthread_mutex_unlock(x) != 0) printf(PMU_ERR)
#define PT_COND_WAIT(x,y) if (pthread_cond_wait(x, y) != 0) printf(PCW_ERR)
#define PT_COND_BROAD(x) if (pthread_cond_broadcast(x) != 0) printf(PCB_ERR)

/* Verbosity levels LOW=info HIGH=info+SQL DEBUG=info+SQL+junk */
enum debugLevel {OFF, LOW, HIGH, DEBUG, DEVELOP}; 

/* These ugly macros keep the code clean and everything inline for speed */
#define sysloginfo(x...) syslog(LOG_INFO |  LOG_LOCAL2, x) // replace with conf'g facility
#define syslogcrit(x...) syslog(LOG_CRIT |  LOG_LOCAL2, x) // replace with conf'g facility
#define debug(level,x...) do {if (set->verbose >= level) {if (set->daemon) sysloginfo(x); else fprintf(stdout,x);} } while (0)
#define tdebug(level,x...) do {if (set->verbose >= level) {if (set->daemon) sysloginfo(x); else {fprintf(stdout, "Thread [%d]: ", worker->index); fprintf(stdout,x);} } } while (0)
#define debugfile(dfp,level,x...) do {if (set->verbose >= level) {if (set->daemon) sysloginfo(x); else fprintf(dfp,x);} } while (0)
#define fatal(x...) do { if (set->daemon) syslogcrit(x); else fprintf(stderr,x); exit(-1); } while (0)
#define fatalfile(dfp,x...) do { fprintf(dfp,x); exit(-1); } while (0)

/* Target state */
enum targetState {NEW, LIVE, STALE};

/* Typedefs */
typedef struct worker_struct {
    int index;
    pthread_t thread;
    struct crew_struct *crew;
} worker_t;

typedef struct config_struct {
    unsigned int interval;
    char dbdriver[MAXPATHLEN];
    char dbhost[80];
    char dbdb[80];
    char dbuser[80];
    char dbpass[80];
    enum debugLevel verbose;
    unsigned short withzeros;
    unsigned short dboff;
    unsigned short multiple;
    unsigned short snmp_port;
    unsigned short threads;
    unsigned short daemon;
    unsigned short syslog;
    float highskewslop;
    float lowskewslop;
} config_t;

typedef struct target_struct {
    struct host_struct *host;
    char *objoid;
    char *description;
    char *table;
    unsigned short bits;
    unsigned int iid;
    int rate;
#ifdef HAVE_STRTOLL
    unsigned long long maxspeed;
#else
    unsigned long maxspeed;
#endif
    enum targetState init;
    unsigned long long last_value;
    struct timeval last_time;
    struct target_struct *next;
} target_t;

typedef struct host_struct {
    char *host;
    char *community;
    unsigned short snmp_ver;
} host_t;

typedef struct crew_struct {
    int work_count;
    int running;
    worker_t member[MAX_THREADS];
    pthread_mutex_t mutex;
    pthread_cond_t done;
    pthread_cond_t go;
} crew_t;

typedef struct poll_stats {
    pthread_mutex_t mutex;
    unsigned long long polls;
    unsigned long long db_inserts;
    unsigned int round;
    unsigned int wraps;
    unsigned int no_resp;
    unsigned int out_of_range;
    unsigned int errors;
    unsigned int slow;
#ifdef FEATURES
    unsigned int zero;
    unsigned int flat;
#endif
    double poll_time; 
} stats_t;

typedef struct hash_struct {
    int bucket;
    target_t *target;
    target_t **table;
} hash_t;

/* Precasts: rtgpoll.c */
void *sig_handler(void *);
void usage(char *);

/* Precasts: rtgpoll.c */
void *poller(void *);

#if HAVE_MYSQL
/* Precasts: rtgmysql.c */
int mysql_db_insert(char *, MYSQL *);
int mysql_dbconnect(char *, MYSQL *);
void mysql_dbdisconnect(MYSQL *);
void mysql_close_db(void *);
#endif

#if HAVE_PGSQL
/* Precasts: rtgpgsql.c */
PGresult * pgsql_db_query(char *, PGconn *);
int pgsql_db_insert(char *, PGconn *);
PGresult * pgsql_db_select(char *, PGconn *);
int pgsql_db_begin(PGconn *);
int pgsql_db_commit(PGconn *);
PGconn * pgsql_dbconnect(char *, PGconn *);
void pgsql_dbdisconnect(PGconn *);
void pgsql_close_db(void *);
#endif

/* Precasts: rtgutil.c */
int read_rtg_config(char *, config_t *);
int write_rtg_config(char *, config_t *);
void config_defaults(config_t *);
void print_stats (stats_t, config_t *);
int sleepy(float, config_t *);
void timestamp(char *);
double timediff(struct timeval, struct timeval);
int checkPID(char *, config_t *);
int daemon_init();

/* Precasts: rtghash.c */
void init_hash();
void init_hash_walk();
target_t *getNext();
void free_hash();
unsigned long make_key(const void *);
void mark_targets(int);
unsigned int delete_targets(int);
void walk_target_hash();
void *in_hash(target_t *, target_t *);
int compare_targets(target_t *, target_t *);
int del_hash_entry(target_t *);
int add_hash_entry(target_t *);
int hash_target_file(char *);

/* extern config_t set; */
extern int lock;
extern int waiting;
extern char config_paths[CONFIG_PATHS][BUFSIZE];
extern FILE *dfp;

#endif /* not _RTG_H_ */
