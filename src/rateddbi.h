/* return codes from db functions that need more options than TRUE/FALSE */
enum DB_RESULT {
    DB_OK,
    DB_FAIL,
    DB_RETRY,
    DB_OOR /* out of range */
};

/* function to determine the driver library and load it */
int db_init(config_t *set);

/* 
 * prototypes for the DBI functions used by the libraries
 * all the librtg*.so files should use these
 */
int __db_test();
int __db_status();
int __db_connect(config_t *config);
int __db_disconnect();
int __db_commit();
enum DB_RESULT  __db_insert(const char *table_esc, unsigned long iid, struct timeval current_time, unsigned long long counter, double rate);
int __db_lookup_oid(const char *, unsigned long *);
char *__db_escape_string(const char *input);
char *__db_check_and_create_data_table(const char *table);
int __db_check_and_create_oids_table(const char *table);

/*
 * we have to jump through some hoops when we load the functions from a library
 */
int (*db_test)();
int (*db_status)();
int (*db_connect)(config_t *config);
int (*db_disconnect)();
int (*db_commit)();
int (*db_insert)(const char *table, unsigned long iid, struct timeval current_time, unsigned long long counter, double rate);
int (*db_lookup_oid)(const char *oid, unsigned long *iid);
char *(*db_escape_string)(const char *input);
char *(*db_check_and_create_data_table)(const char *table);
int (*db_check_and_create_oids_table)(const char *table);
