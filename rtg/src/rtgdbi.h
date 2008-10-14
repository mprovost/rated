#include "rtg.h"
#include "rtgplot.h"

/* function to determine the driver library and load it */
int db_init(config_t *set);

/* 
 * prototypes for the DBI functions used by the libraries
 * all the librtg*.so files should use these
 */
int __db_test();
int __db_status();
int __db_populate(char *query,data_obj_t *DO);
#ifdef HAVE_STRTOLL
long long __db_intSpeed(char *query);
#else
long __db_intSpeed(char *query);
#endif

int __db_connect(config_t *config);
int __db_disconnect();
int __db_commit();
int __db_insert(char *table, int iid, unsigned long long counter, double rate);

/*
 * we have to jump through some hoops when we load the functions from a library
 */
int (*db_test)();
int (*db_status)();
int (*db_populate)(char *query,data_obj_t *DO);
#ifdef HAVE_STRTOLL
long long (*db_intSpeed)(char *query);
#else
long (*db_intSpeed)(char *query);
#endif
int (*db_connect)(config_t *config);
int (*db_disconnect)();
int (*db_commit)();
int (*db_insert)(char *table, int iid, unsigned long long counter, double rate);
