/****************************************************************************
   Program:     $Id: rtgpgsql.c,v 1.6 2008/01/19 03:01:32 btoneill Exp $
   Original:    Matt Provost
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Purpose:     RTG Postgres routines
****************************************************************************/

#include "common.h"
#include "rtg.h"

#if HAVE_PGSQL
extern FILE *dfp;
extern config_t *set;

/* generic function to run queries */
PGresult * pgsql_db_query(char *query, PGconn * pgsql)
{
    PGresult *result;
    ExecStatusType status;

    /* see if we connected ok */
    if (PQstatus(pgsql) != CONNECTION_OK) {
	fprintf(dfp, "** Failed: %s\n", PQerrorMessage(pgsql));
	/* make an empty error result */
	result = PQmakeEmptyPGresult(pgsql, PGRES_FATAL_ERROR);
	/* close the connection */
	PQfinish(pgsql);
	return (result);
    } else {
		debugfile(dfp, HIGH, "DB Connection OK\n");
    }

	debugfile(dfp, HIGH, "SQL: %s\n", query);
    /* run the query */
    result = PQexec(pgsql, query);

    /* error check in the wrapper functions */

    return (result);
}

/* wrapper for inserts */
int pgsql_db_insert(char *query, PGconn * pgsql)
{
    PGresult *result;
    ExecStatusType status;

    result = pgsql_db_query(query, pgsql);

    /* check the result */
    status = PQresultStatus(result);

    if (status != PGRES_COMMAND_OK) {
	if (set->verbose >= LOW) {
	    fprintf(stderr, "** Postgres Insert Error: %s\n", PQresultErrorMessage(result));
	    /* clear the result */
	    PQclear(result);
	}
	return (FALSE);
    } else
	/* clear the result */
	PQclear(result);
	return (TRUE);
}


/* wrapper for selects */
PGresult * pgsql_db_select(char *query, PGconn * pgsql)
{
    PGresult *result;
    ExecStatusType status;

    result = pgsql_db_query(query, pgsql);

    /* check the result */
    status = PQresultStatus(result);

    if (status != PGRES_TUPLES_OK) {
	if (set->verbose >= LOW) {
	    fprintf(stderr, "** Postgres Select Error: %s\n", PQresultErrorMessage(result));
	    /* clear the result */
	    PQclear(result);
	    /* make an empty error result */
            result = PQmakeEmptyPGresult(pgsql, PGRES_FATAL_ERROR);
	}
	return (result);
    } else
	/* return the result */
	return (result);
}

int pgsql_db_begin(PGconn * pgsql)
{
    PGresult *result;
    ExecStatusType status;

    result = pgsql_db_query("BEGIN", pgsql);

    /* check the result */
    status = PQresultStatus(result);

    if (status != PGRES_COMMAND_OK) {
	if (set->verbose >= LOW) {
	    fprintf(stderr, "** Postgres Transaction Begin Error: %s\n", PQresultErrorMessage(result));
	}
	return(FALSE);
    } else {
	return(TRUE);
    }
}

int pgsql_db_commit(PGconn * pgsql)
{
    PGresult *result;
    ExecStatusType status;

    result = pgsql_db_query("COMMIT", pgsql);

    /* check the result */
    status = PQresultStatus(result);

    if (status != PGRES_COMMAND_OK) {
	if (set->verbose >= LOW) {
	    fprintf(stderr, "** Postgres Transaction Begin Error: %s\n", PQresultErrorMessage(result));
	}
	return(FALSE);
    } else {
	return(TRUE);
    }
}

PGconn * pgsql_dbconnect(char *database, PGconn *pgsql)
{
    char conninfo[256];

    /* construct a connection string */
    snprintf(conninfo,256 , "host=%s user=%s password=%s dbname=%s", set->dbhost, set->dbuser, set->dbpass, database);
	debugfile(dfp, LOW, "Connecting to Postgres database '%s'...\n", conninfo);
    /* connect */
    pgsql = PQconnectdb(conninfo);
    /* see if we connected ok */
    if (PQstatus(pgsql) != CONNECTION_OK) {
	fprintf(dfp, "** Failed: %s\n", PQerrorMessage(pgsql));
	/* close the connection */
	PQfinish(pgsql);
	return (pgsql);
    } else {
		debugfile(dfp, LOW, "DB Connection OK\n");
	}
	return (pgsql);
}


void pgsql_dbdisconnect(PGconn * pgsql)
{
    PQfinish(pgsql);
}

void pgsql_close_db(void *arg)
{
    /* to be called from the cleanup handler */
	debug(LOW, "Closing database connection\n");
    PQfinish(arg);
}

#endif /* HAVE_PGSQL */
