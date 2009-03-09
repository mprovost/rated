/****************************************************************************
   Program:     $Id: rtgmysql.c,v 1.18 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Purpose:     RTG MySQL routines
****************************************************************************/

#include "common.h"
#include "rtg.h"

#if HAVE_MYSQL
extern FILE *dfp;
extern config_t *set;

int mysql_db_insert(char *query, MYSQL * mysql)
{
    if (mysql_query(mysql, query)) {
		debug(LOW, "** MySQL Error: %s\n", mysql_error(mysql));
		return (FALSE);
    }
	return (TRUE);
}


int mysql_dbconnect(char *database, MYSQL * mysql)
{
    if (set->verbose >= LOW) {
		if(set->daemon)
			sysloginfo("Connecting to MySQL database '%s' on '%s'...", database, set->dbhost);
		else
			fprintf(dfp, "Connecting to MySQL database '%s' on '%s'...", database, set->dbhost);
	}
    mysql_init(mysql);
    if (!mysql_real_connect
     (mysql, set->dbhost, set->dbuser, set->dbpass, database, 0, NULL, 0)) {
	fprintf(dfp, "** Failed: %s\n", mysql_error(mysql));
	return (-1);
    } else
	return (0);
}


void mysql_dbdisconnect(MYSQL * mysql)
{
    mysql_close(mysql);
}

void mysql_close_db(void *arg)
{
	/* to be called from the cleanup handler */
	if (MYSQL_VERSION_ID > 40000)
		mysql_thread_end();
	else
		my_thread_end();
	mysql_close(arg);
}

#endif /* HAVE_MYSQL */
