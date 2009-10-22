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

	/* TODO check return */
	/* escape the string */
	scratch_len = PQescapeStringConn(pgsql, scratch, input, input_len, NULL);

        debug(DEBUG, "scratch = %s\n", scratch);

        output = strdup(scratch);

        debug(DEBUG, "output = %s\n", output);

	free(scratch);

        debug(DEBUG, "output = %s\n", output);

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

int __db_connect(config_t *config) {
	char *connectstring;
	PGconn* pgsql;

	set = config;

	/* TODO escape strings */
	asprintf(&connectstring, "host='%s' dbname='%s' user='%s' password='%s'", set->dbhost, set->dbdb, set->dbuser, set->dbpass);

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
int __db_insert(const char *table_esc, unsigned long iid, unsigned long long insert_val, double insert_rate) {
	PGconn *pgsql = getpgsql();

	char *query;
	ExecStatusType status;

	PGresult *result;

        if (pgsql == NULL) {
            debug(LOW, "No Postgres connection in db_insert\n");
            return FALSE;
        }

	/* INSERT INTO %s (id,dtime,counter,rate) VALUES (%d, NOW(), %llu, %.6f) */
        /* don't include the rate column if it's not needed */
        if (insert_rate > 0) {
            /* double columns have precision of at least 15 digits */
            asprintf(&query,
                "INSERT INTO \"%s\" (id,dtime,counter,rate) VALUES (%lu,NOW(),%llu,%.15f)",
                table_esc, iid, insert_val, insert_rate);
        } else {
            asprintf(&query,
                "INSERT INTO \"%s\" (id,dtime,counter) VALUES (%lu,NOW(),%llu)",
                table_esc, iid, insert_val);
        }

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

/*
 * insert an (escaped) snmp oid into the database and update the iid
 * this probably only gets called by __db_lookup_oid 
 */
int db_insert_oid(PGconn *pgsql, const char *oid_esc, unsigned long *iid) {
    int status;
    char *query;
    PGresult *result;

    asprintf(&query,
        "INSERT INTO \"oids\" (oid) VALUES (\'%s\') RETURNING iid",
        oid_esc);

    debug(HIGH, "Query = %s\n", query);

    result = PQexec(pgsql, query);

    free(query);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        *iid = strtoul(PQgetvalue(result, 0, 0), NULL, 0);
        status = TRUE;
    } else {
        debug(LOW, "Postgres %s", PQerrorMessage(pgsql));
        status = FALSE;
    }

    (void)PQclear(result);

    return status;
}

/* lookup the iid of an snmp oid in the database */
int __db_lookup_oid(const char *oid, unsigned long *iid) {
    PGconn *pgsql = getpgsql();

    int status;
    char *query;
    char *oid_esc;
    PGresult *result;

    if (pgsql == NULL) {
        debug(LOW, "No Postgres connection in db_lookup_oid\n");
        return 0;
    }

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
                debug(LOW, "ntuples = 1\n");
                *iid = strtoul(PQgetvalue(result, 0, 0), NULL, 0);
                status = TRUE;
                break;
            case 0:
                debug(LOW, "ntuples = 0\n");
                status = -1; /* do an insert below */
                break;
            default:
                /* this shouldn't happen */
                debug(LOW, "ntuples = %i\n", PQntuples(result));
                status = FALSE;
        }
    } else {
        debug(LOW, "Postgres %s", PQerrorMessage(pgsql));
        status = FALSE;
    }

    /* free the result before we reuse the connection */
    (void)PQclear(result);

    if (status == -1) {
        status = db_insert_oid(pgsql, oid_esc, iid);
    }

    free(oid_esc);

    return status;
}

/* internal function to check if a table exists in the database
 * takes an unescaped table name and returns an escaped one or null */
char *__db_check_table(const char *table) {
    PGconn *pgsql = getpgsql();
    char *table_esc;
    char *query;
    char *db;
    PGresult *result;

    table_esc = escape_string(pgsql, table);

    db = PQdb(pgsql);

    asprintf(&query,
        "SELECT \"table_name\" FROM information_schema.tables WHERE table_catalog = '%s' AND table_schema = 'public' AND table_name = '%s'",
        db, table_esc);

    debug(HIGH, "Query = %s\n", query);
    result = PQexec(pgsql, query);
    free(query);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        if (PQntuples(result) == 1) {
            debug(HIGH, "%s found!\n", table_esc);
        } else {
            debug(HIGH, "%s missing!\n", table_esc);
            /* we want to return NULL in this case */
            free(table_esc);
            table_esc = NULL; /* needed? */
        }
    } else {
        debug(LOW, "Postgres %s", PQerrorMessage(pgsql));
        /* we want to return NULL in this case */
        free(table_esc);
        table_esc = NULL; /* needed? */
    }

    (void)PQclear(result);

    return table_esc;
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

/* create a data table
 * takes an escaped table name */
/* TODO check the table name length against NAMEDATALEN -1 (ie 63) */
int __db_create_data_table(const char *table_esc) {
    PGconn *pgsql = getpgsql();
    int status;
    char *query;
    const char *create = 
    "CREATE TABLE \"%s\" ("
    "id int NOT NULL default '0',"
    "dtime timestamp NOT NULL,"
    "counter bigint NOT NULL default '0',"
    "rate real NOT NULL default '0.0'"
    ")";
    const char *index = "CREATE INDEX \"%s_idx\" ON \"%s\" (id,dtime)";

    asprintf(&query, create, table_esc);
    debug(HIGH, "Query = %s\n", query);

    if (db_exec_command(pgsql, query)) {
        free(query);
        asprintf(&query, index, table_esc, table_esc);
        debug(HIGH, "Query = %s\n", query);
        if (db_exec_command(pgsql, query)) {
            status = TRUE;
        } else {
            status = FALSE;
        }
    }
    /* this will either be the create table query if that failed, or the index query if the create succeeded */ 
    free(query);

    return status;
}

int __db_create_oids_table(const char *table_esc) {
    PGconn *pgsql = getpgsql();
    int status;
    char *query;
    const char *create = 
    "CREATE TABLE \"%s\" ("
    "iid SERIAL PRIMARY KEY,"
    "oid TEXT NOT NULL UNIQUE"
    ")";

    asprintf(&query, create, table_esc);
    debug(HIGH, "Query = %s\n", query);

    if (db_exec_command(pgsql, query)) {
        status = TRUE;
    } else {
        status = FALSE;
    }
    free(query);

    return status;
}
