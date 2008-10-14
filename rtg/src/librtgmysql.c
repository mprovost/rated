/*
 * RTG MySQL database driver
 */

#include "rtg.h"
#include "rtgdbi.h"

#include <mysql.h>

#include <sys/param.h>

/* thread-specific global variable */
pthread_key_t key;

config_t *set;

/* variable cleanup function */
void killkey(void *target) {
	free(target);
}

/* this gets called once from dl_init */
void my_makekey() {
	/* this shouldn't fail, and we're too early on to report errors */
	pthread_key_create(&key, killkey);
}

/* called when library loads */
void __attribute__ ((constructor)) dl_init(void) {
	/* only call the thread-specific variable setup once */
	pthread_once_t  once = PTHREAD_ONCE_INIT;

	pthread_once(&once, my_makekey);

}

int __db_test() {
	return TRUE;
}

/* return the thread-specific mysql variable */
MYSQL getmysql() {
	MYSQL *mysql;
	/* if this fails it will just return NULL */
	mysql = pthread_getspecific(key);

	return(*mysql);
}

/* utility function to safely escape table names */
char *escape_string(char *output, char *input)
{
	MYSQL mysql = getmysql();

	/* length of string */
	size_t input_len = strlen(input);

	/* target for mysql_real_escape_string */
	/* worst case is every char escaped plus terminating NUL */
	char *scratch = malloc(input_len*2+1);

	/* TODO check return */
	/* escape the string */
	mysql_real_escape_string(&mysql, scratch, input, input_len);

	/* set output to correct length string, including NUL */
	asprintf(&output, "%s", scratch);

	free(scratch);

	return output;
}

/*
 * check the status of the connection
 * we don't try and reconnect because this is sometimes used to confirm a disconnect
 */
int __db_status() {
	MYSQL mysql = getmysql();

	if (mysql_ping(&mysql) == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int __db_connect(config_t *config) {
	MYSQL* mysql;

	/* set the global config_t pointer so all functions can use it */
	set = config;

	/* reserve space for the mysql variable */
	mysql = (MYSQL*)malloc(sizeof(MYSQL));
	if (!mysql) {
		debug(LOW, "malloc: out of memory\n");
		return FALSE;
	}

	/* first check that we are using a threaded client */
	if (mysql_thread_safe() == 0) {
		debug(LOW, "MySQL library isn't thread safe\n");
		return FALSE;
	}

	/* initialize the connection struct */
	if (mysql_init(mysql) == NULL) {
		return FALSE;
	}

	/* shouldn't have to call these, but... */
	if (MYSQL_VERSION_ID > 40000) {
		mysql_thread_init();
	} else {
		my_thread_init();
	}

	/* TODO do we need a sigpipe handler to be totally threadsafe? */

	/* read any options from the my.cnf file under the rtg heading */
	mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "rtg");

	/* MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long client_flag) */
	if (!mysql_real_connect(mysql, set->dbhost, set->dbuser, set->dbpass, set->dbdb, 0, NULL, 0)) {
		debug(LOW, "Failed to connect to mysql server: %s\n", mysql_error(mysql));
	} else {
		debug(LOW, "Mysql connected, thread: %u\n", mysql_thread_id(mysql));
	}

	/* put the mysql connection back into the global var */
	if (!pthread_setspecific(key, (void*)mysql) == 0) {
		debug(LOW, "Couldn't set thread specific storage\n");
		return FALSE;
	}

	return __db_status();
}

int __db_disconnect() {
	MYSQL mysql = getmysql();

	/* no return value to check */
	mysql_close(&mysql);

	debug(LOW, "Mysql connection closed\n");

	return TRUE;
}

int __db_insert(char *table, int iid, unsigned long long insert_val, double insert_rate) {
	MYSQL mysql = getmysql();

	char *query;

	char *table_esc;

	int result;

	table_esc = escape_string(table_esc, table);

	asprintf(&query, 
		"INSERT INTO `%s` (id,dtime,counter,rate) VALUES (%i,NOW(),%llu,%.6f)",
		table_esc, iid, insert_val, insert_rate);

	free(table_esc);

	debug(HIGH, "Query = \"%s\"\n", query);

	/* now execute the query */

	result = mysql_real_query(&mysql, query, strlen(query));

	free(query);

	if (result == 0) {
		return TRUE;
	} else {
		debug(LOW, "MySQL error: %s\n", mysql_error(&mysql));
		return FALSE;
	}
}

int __db_commit() {
    struct timespec ts1;
    struct timespec ts2;
    unsigned int ms_took;
    int com_ret;
    MYSQL mysql = getmysql();

    clock_gettime(CLOCK_REALTIME,&ts1);
    
    com_ret = mysql_commit(&mysql);

    clock_gettime(CLOCK_REALTIME,&ts2);
    
    ms_took = (unsigned int)((ts2.tv_sec * 1000000000 + ts2.tv_nsec) - (ts1.tv_sec * 1000000000 + ts1.tv_nsec)) / 1000000;
    debug(HIGH,"Commit took %d milliseconds at [%d][%d][%d][%d]\n",ms_took,ts1.tv_sec,ts1.tv_nsec,ts2.tv_sec,ts2.tv_nsec);

    return com_ret;
}
