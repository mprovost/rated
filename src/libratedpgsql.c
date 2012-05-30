/*
 * RTG PostgresSQL database driver
 */
  
#include "rated.h"
#include "rateddbi.h"

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
char *escape_string(PGconn *pgsql, const char *input)
{
	/* length of string */
	size_t input_len = strlen(input);
        char *scratch;
        size_t scratch_len;
        char *output;

        /* worst case is every char escaped plus terminating NUL */
        scratch = malloc(input_len*2+1);

	/* escape the string */
	scratch_len = PQescapeStringConn(pgsql, scratch, input, input_len, NULL);
        if (scratch_len) {
            output = strndup(scratch, scratch_len);
            free(scratch);
        } else {
            output = NULL;
        }

        debug(DEBUG, "escape_string input = '%s' output = '%s'\n", input, output);

        return output;
}

/* wrapper function for escaping strings to be called externally */
char *__db_escape_string(const char *input) {
	PGconn *pgsql = getpgsql();

        return escape_string(pgsql, input);
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

/*
 * connect to the database server
 * if there is already an existing connection, just return
 */
int __db_connect(config_t *config) {
	char *connectstring;
	PGconn* pgsql = getpgsql();

	set = config;

        /* shortcut */
	if (PQstatus(pgsql) == CONNECTION_OK)
            return TRUE;

	/* TODO escape strings */
	asprintf(&connectstring, "host='%s' dbname='%s' user='%s' password='%s'", set->dbhost, set->dbdb, set->dbuser, set->dbpass);
        debug(HIGH, "Connecting to postgres %s@%s:%s\n", set->dbuser, set->dbhost, set->dbdb);

	pgsql = PQconnectdb(connectstring);

	free(connectstring);

	if (PQstatus(pgsql) == CONNECTION_OK) {
		debug(LOW, "Postgres connected, PID: %u\n", PQbackendPID(pgsql));
	} else {
		debug(LOW, "Failed to connect to postgres server: %s", PQerrorMessage(pgsql));
                PQfinish(pgsql);
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

        if (pgsql) {
	    PQfinish(pgsql);
	    debug(LOW, "Disconnected from postgres\n");

	    /*
	     * PQfinish free()s the memory location stored in the key,
	     * so when the thread shuts down we get a double free and
	     * a warning. Setting it back to NULL avoids this.
	     */
	    pthread_setspecific(key, NULL);
        }

	return TRUE;
}

/*
 * insert a row into the db
 * this expects an escaped table name
 */
int __db_insert(const char *table_esc, unsigned long iid, struct timeval current_time, unsigned long long insert_val, double insert_rate) {
	PGconn *pgsql = getpgsql();

	char *query;
        char now[20];
	int status;

	PGresult *result;

        if (pgsql == NULL) {
            debug(LOW, "No Postgres connection in db_insert\n");
            return FALSE;
        }

        tv2iso8601(now, current_time);

	/* INSERT INTO %s (iid,dtime,counter,rate) VALUES (%d, NOW(), %llu, %.6f) */
        /* don't include the rate column if it's not needed */
        if (insert_rate > 0) {
            /* double columns have precision of at least 15 digits */
            asprintf(&query,
                "INSERT INTO \"%s\" (iid,dtime,counter,rate) VALUES (%lu,\'%s\',%llu,%.15f)",
                table_esc, iid, now, insert_val, insert_rate);
        } else {
            asprintf(&query,
                "INSERT INTO \"%s\" (iid,dtime,counter) VALUES (%lu,\'%s\',%llu)",
                table_esc, iid, now, insert_val);
        }

	debug(HIGH, "Query = %s\n", query);

	result = PQexec(pgsql, query);

        if (PQresultStatus(result) == PGRES_COMMAND_OK) {
            status = TRUE;
        } else {
            status = FALSE;
            /* Note that by libpq convention, a non-empty PQerrorMessage will include a trailing newline. */
            /* also errors start with 'ERROR:' so we don't need to */
            debug(LOW, "Postgres %s%s\n", PQerrorMessage(pgsql), query);
	}

	/* free the result */
	(void)PQclear(result);
	free(query);

        return(status);
}

/*
 * insert an (escaped) snmp oid into the database and update the iid
 */
int db_insert_oid(PGconn *pgsql, const char *oid_esc, unsigned long *iid) {
    int status = FALSE;
    char *query;
    PGresult *result;


    asprintf(&query,
        "INSERT INTO \"oids\" (oid) (SELECT \'%s\' AS oid WHERE NOT EXISTS (SELECT 1 FROM \"oids\" WHERE oid=\'%s\')) RETURNING iid",
        oid_esc, oid_esc);
    debug(HIGH, "Query = %s\n", query);

    result = PQexec(pgsql, "BEGIN");
    (void)PQclear(result);
    result = PQexec(pgsql, query);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        if (PQntuples(result)) {
            *iid = strtoul(PQgetvalue(result, 0, 0), NULL, 0);
            status = TRUE;
        } else {
            /* no result so may have been a concurrent insert, do a select instead */
            /* don't call db_lookup_oid because that calls back here */
            free(query);
            asprintf(&query,
                "SELECT \"iid\" from \"oids\" WHERE \"oid\" = \'%s\'",
                oid_esc);
            debug(HIGH, "Query = %s\n", query);
            (void)PQclear(result);
            result = PQexec(pgsql, query);
            if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result)) {
                *iid = strtoul(PQgetvalue(result, 0, 0), NULL, 0);
                status = TRUE;
            }
        }
    }

    if (status == FALSE) {
        debug(LOW, "Postgres %s%s\n", PQerrorMessage(pgsql), query);
        status = FALSE;
    }

    (void)PQclear(result);
    result = PQexec(pgsql, "END");
    (void)PQclear(result);
    free(query);

    return status;
}

/* lookup the iid of an snmp oid in the database */
int __db_lookup_oid(const char *oid, unsigned long *iid) {
    PGconn *pgsql = getpgsql();
    int status;
    char *query;
    char *oid_esc;
    PGresult *result;

    oid_esc = escape_string(pgsql, oid);
    debug(DEBUG, "oid_esc = %s\n", oid_esc);

    asprintf(&query,
        "SELECT \"iid\" from \"oids\" WHERE \"oid\" = \'%s\'",
        oid_esc);
    debug(HIGH, "Query = %s\n", query);

    result = PQexec(pgsql, query);
    free(query);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        switch (PQntuples(result)) {
            case 1:
                *iid = strtoul(PQgetvalue(result, 0, 0), NULL, 0);
                debug(DEBUG, "iid = %lu\n", *iid);
                status = TRUE;
                break;
            case 0:
                status = db_insert_oid(pgsql, oid_esc, iid);
                break;
            default:
                /* this shouldn't happen */
                debug(DEBUG, "oid '%s' ntuples = %i\n", oid_esc, PQntuples(result));
                status = FALSE;
        }
    } else {
        debug(LOW, "Postgres %s", PQerrorMessage(pgsql));
        status = FALSE;
    }

    (void)PQclear(result);
    free(oid_esc);

    return status;
}

/* internal function to check if a table exists in the database
 * takes an unescaped table name and returns an escaped one or null */
int db_unsafe_check_table(PGconn *pgsql, const char *table) {
    int status;
    char *query;
    char *db;
    PGresult *result;

    db = PQdb(pgsql);

    asprintf(&query,
        "SELECT \"table_name\" FROM information_schema.tables WHERE table_catalog = '%s' AND table_schema = current_schema() AND table_name = '%s'",
        db, table);

    debug(HIGH, "Query = %s\n", query);
    result = PQexec(pgsql, query);
    free(query);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        if (PQntuples(result) == 1) {
            debug(DEBUG, "%s found!\n", table);
            status = TRUE;
        } else {
            debug(DEBUG, "%s missing!\n", table);
            status = FALSE;
        }
    } else {
        debug(LOW, "Postgres %s", PQerrorMessage(pgsql));
        status = FALSE;
    }

    (void)PQclear(result);

    return status;
}

/* execute a sql command (query) that doesn't return any rows */
int db_exec_command(PGconn *pgsql, const char *query) {
    PGresult *result;
    int status;

    result = PQexec(pgsql, query);

    if (PQresultStatus(result) == PGRES_COMMAND_OK) {
        status = TRUE;
    } else {
        debug(LOW, "Postgres %s", PQerrorMessage(pgsql));
        status = FALSE;
    }

    (void)PQclear(result);

    return status;
}

/* check if a data table exists and if not, create it
 * return the escaped table name */
/* TODO check the table name length against NAMEDATALEN -1 (ie 63) */
char *__db_check_and_create_data_table(const char *table) {
    PGconn *pgsql = getpgsql();
    char *query;
    char *table_esc;
    const char *create = 
    "CREATE TABLE \"%s\" ("
    "iid int NOT NULL default '0',"
    "dtime timestamp NOT NULL,"
    "counter bigint NOT NULL default '0',"
    "rate real NOT NULL default '0.0'"
    ")";
    const char *index = "CREATE INDEX \"%s_idx\" ON \"%s\" (iid,dtime)";

    table_esc = escape_string(pgsql, table);
    /* first check if it already exists */
    if (!db_unsafe_check_table(pgsql, table_esc)) {
        asprintf(&query, create, table_esc);
        debug(LOW, "\'%s\' table not found, creating\n", table_esc);
        debug(HIGH, "Query = %s\n", query);

        if (db_exec_command(pgsql, query)) {
            free(query);
            asprintf(&query, index, table_esc, table_esc);
            debug(HIGH, "Query = %s\n", query);
            if (!db_exec_command(pgsql, query)) {
                free(table_esc);
                table_esc = NULL;
            }
        } else {
            free(table_esc);
            table_esc = NULL;
        }
        /* this will either be the create table query if that failed, or the index query if the create succeeded */ 
        free(query);
    }

    return table_esc;
}

/* check if the oids table exists and if not, create it */
/* the oids table name is a compiled constant so we assume it's safe */
int __db_check_and_create_oids_table(const char *table) {
    PGconn *pgsql = getpgsql();
    int status;
    char *query;
    const char *create = 
    "CREATE TABLE \"%s\" ("
    "iid SERIAL PRIMARY KEY,"
    "oid TEXT NOT NULL UNIQUE"
    ")";

    if (db_unsafe_check_table(pgsql, table)) {
        status = TRUE;
    } else {
        asprintf(&query, create, table);
        debug(LOW, "oids table not found, creating\n");
        debug(HIGH, "Query = %s\n", query);

        if (db_exec_command(pgsql, query)) {
            status = TRUE;
        } else {
            status = FALSE;
        }
        free(query);
    }

    return status;
}
