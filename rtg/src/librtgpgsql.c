/*
 * RTG PostgresSQL database driver
 */
  
#include "rtg.h"
#include "rtgdbi.h"

#include <libpq-fe.h>

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
	/* this shouldn't fail and we're too early to return an error */
	pthread_key_create(&key, killkey);
}
			  
/* called when library loads */
void __attribute__ ((constructor)) dl_init(void) {
/* only call the thread-specific variable setup once */
	pthread_once_t  once = PTHREAD_ONCE_INIT;

	pthread_once(&once, my_makekey);

}

/* return the thread-specific pgsql variable */
PGconn* getpgsql() {
        PGconn *pgsql;
	pgsql = pthread_getspecific(key);

	return(pgsql);
}

/* utility function to safely escape table names */
char *escape_string(char *output, char *input)
{
	/* length of string */
	size_t input_len = strlen(input);

	/* target for PQescapeString */
	/* worst case is every char escaped plus terminating NUL */
	char *scratch = malloc(input_len*2+1);

	/* TODO check return */
	/* escape the string */
	PQescapeString(scratch, input, input_len);

	/* set output to correct length string */
	asprintf(&output, "%s", scratch);

	free(scratch);

	return output;
}


int __db_test() {
	return 1;
}

int __db_status() {
	PGconn *pgsql = getpgsql();

	if (PQstatus(pgsql) == CONNECTION_OK) {
		return TRUE;
	} else {
		debug(LOW, "Postgres error: %s\n", PQerrorMessage(pgsql));
		return FALSE;
	}
}

int __db_connect(config_t *config) {
	char *connectstring;
	PGconn* pgsql;

	set = config;

	/* reserve space for the pgsql variable */
	pgsql = (PGconn*)malloc(sizeof(PGconn*));
	if (!pgsql) {
		debug(LOW, "malloc: out of memory\n");
		return FALSE;
	}

	/* TODO escape strings */
	asprintf(&connectstring, "host='%s' dbname='%s' user='%s' password='%s'", set->dbhost, set->dbdb, set->dbuser, set->dbpass);

	pgsql = PQconnectdb(connectstring);

	free(connectstring);

	if (PQstatus(pgsql) == CONNECTION_OK) {
		debug(LOW, "Postgres connected, PID: %u\n", PQbackendPID(pgsql));
	} else {
		debug(LOW, "Failed to connect to postgres server: %s", PQerrorMessage(pgsql));
		return FALSE;
	}

	/* put the connection into thread-local storage */
	if (!pthread_setspecific(key, (void*)pgsql) == 0) {
		debug(LOW, "Couldn't set thread specific storage\n");
		return FALSE;
	}

	return TRUE;

}

int __db_disconnect() {
	PGconn *pgsql = getpgsql();

	debug(LOW, "Disconnecting from postgres\n");
	PQfinish(pgsql);

	/*
	 * PQfinish free()s the memory location stored in the key,
	 * so when the thread shuts down we get a double free and
	 * a warning. Setting it back to NULL avoids this.
	 */
	pthread_setspecific(key, NULL);

	return TRUE;
}


int __db_insert(char *table, int iid, unsigned long long insert_val, double insert_rate) {
	PGconn *pgsql = getpgsql();

	char *query;
	char *table_esc;
	ExecStatusType status;

	PGresult *result;

	/* size_t PQescapeString (char *to, const char *from, size_t length); */
	/* INSERT INTO %s (id,dtime,counter,rate) VALUES (%d, NOW(), %llu, %.6f) */
	/* escape table name */
	table_esc = escape_string(table_esc, table);
				
	asprintf(&query,
		"INSERT INTO \"%s\" (id,dtime,counter,rate) VALUES (%d,NOW(),%llu,%.6f)",
		table_esc, iid, insert_val, insert_rate);

	free(table_esc);

	debug(HIGH, "Query = %s\n", query);

	result = PQexec(pgsql, query);

	free(query);

	status = PQresultStatus(result);

	/* free the result */
	(void)PQclear(result);

	if (status == PGRES_COMMAND_OK) {
		return TRUE;
	} else {
		/* Note that by libpq convention, a non-empty PQerrorMessage will include a trailing newline. */
		/* also errors start with 'ERROR:' so we don't need to */
		debug(LOW, "Postgres %s", PQerrorMessage(pgsql));

		return FALSE;
	}
}
