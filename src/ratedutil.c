/****************************************************************************
   Program:     $Id: rtgutil.c,v 1.35 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: rated Routines
****************************************************************************/

#include "common.h"
#include "rated.h"

#include <sys/stat.h>

extern sig_atomic_t quitting;

extern FILE *dfp;

/* read configuration file to establish local environment */
int read_rated_config(char *file, config_t * set)
{
    FILE *fp;
    char buff[BUFSIZE];
    char p1[BUFSIZE];
    char p2[BUFSIZE];

    if ((fp = fopen(file, "r")) == NULL) {
        return (-1);
    } else {
		if (set->verbose >= LOW) {
			if (set->daemon)
				sysloginfo("Using rated config file [%s].\n", file);
			else
				fprintf(dfp, "Using rated config file [%s].\n", file);
		}
        while(!feof(fp)) {
           fgets(buff, BUFSIZE, fp);
           if (!feof(fp) && *buff != '#' && *buff != ' ' && *buff != '\n') {
              sscanf(buff, "%20s %20s", p1, p2);
              if (!strcasecmp(p1, "Interval")) set->interval = atoi(p2) * 1000; /* convert to milliseconds */
              else if (!strcasecmp(p1, "SNMP_Port")) set->snmp_port = atoi(p2);
              else if (!strcasecmp(p1, "Threads")) set->threads = atoi(p2);
              else if (!strcasecmp(p1, "DB_Driver")) strncpy(set->dbdriver, p2, sizeof(set->dbdriver));
              else if (!strcasecmp(p1, "DB_Host")) strncpy(set->dbhost, p2, sizeof(set->dbhost));
              else if (!strcasecmp(p1, "DB_Database")) strncpy(set->dbdb, p2, sizeof(set->dbdb));
              else if (!strcasecmp(p1, "DB_User")) strncpy(set->dbuser, p2, sizeof(set->dbuser));
              else if (!strcasecmp(p1, "DB_Pass")) strncpy(set->dbpass, p2, sizeof(set->dbpass));
              else { 
                 fatalfile(dfp, "*** Unrecongized directive: %s=%s in %s\n", 
                    p1, p2, file);
              }
           }
        }
        if (set->threads < 1 || set->threads > MAX_THREADS) 
          fatalfile(dfp, "*** Invalid Number of Threads: %d (max=%d).\n", 
             set->threads, MAX_THREADS);
        return (0);
    }
}


int write_rated_config(char *file, config_t * set)
{
    FILE *fp;

	if (set->verbose >= LOW && !(set->daemon))
		fprintf(dfp, "Writing default config file [%s].", file);
    if ((fp = fopen(file, "w")) == NULL) {
        fprintf(dfp, "\nCould not open '%s' for writing\n", file);
        return (-1);
    } else {
        fprintf(fp, "#\n# RTG v%s Master Config\n#\n", VERSION);
        fprintf(fp, "Interval\t%d\n", set->interval);
        fprintf(fp, "SNMP_Port\t%d\n", set->snmp_port);
        fprintf(fp, "DB_Driver\t%s\n", set->dbdriver);
        fprintf(fp, "DB_Host\t%s\n", set->dbhost);
        fprintf(fp, "DB_Database\t%s\n", set->dbdb);
        fprintf(fp, "DB_User\t%s\n", set->dbuser);
        fprintf(fp, "DB_Pass\t%s\n", set->dbpass);
        fprintf(fp, "Threads\t%d\n", set->threads);
        fclose(fp);
        return (0);
    }
}


/* Populate Master Configuration Defaults */
void config_defaults(config_t * set)
{
   set->interval = DEFAULT_INTERVAL;
   set->snmp_port = DEFAULT_SNMP_PORT;
   set->threads = DEFAULT_THREADS;
   strncpy(set->dbdriver, DEFAULT_DB_DRIVER, sizeof(set->dbdriver));
   strncpy(set->dbhost, DEFAULT_DB_HOST, sizeof(set->dbhost));
   strncpy(set->dbdb, DEFAULT_DB_DB, sizeof(set->dbdb));
   strncpy(set->dbuser, DEFAULT_DB_USER, sizeof(set->dbuser));
   strncpy(set->dbpass, DEFAULT_DB_PASS, sizeof(set->dbpass));
   set->dboff = FALSE;
   set->withzeros = FALSE;
   set->verbose = OFF; 
   set->daemon = TRUE;
   strncpy(config_paths[0], CONFIG_PATH_1, sizeof(config_paths[0]));
   strncpy(config_paths[1], CONFIG_PATH_2, sizeof(config_paths[1]));
   snprintf(config_paths[2], sizeof(config_paths[1]), "%s/etc/", RTG_HOME);
   return;
}


/* Print RTG poll stats */
void print_stats(stats_t stats, config_t *set)
{
  debug(OFF, "[Polls = %lld] [DBInserts = %lld] [DBErrors = %lld] [Zero = %d] [Wraps = %d]\n",
      stats.polls, stats.db_inserts, stats.db_errors, stats.zero, stats.wraps);
  debug(OFF, "[NoResp = %d] [SNMPErrors = %d] [Slow = %d] [PollTime = %u ms]\n",
      stats.no_resp, stats.errors, stats.slow, stats.poll_time);
  return;
}


/* A fancy sleep routine */
/* sleep time is in milliseconds */
int sleepy(unsigned int sleep_time, config_t *set)
{
    int i;
    struct timespec ts;

    /* convert to seconds */
    sleep_time = sleep_time / 1000;

    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    debug(LOW, "Sleeping for %u seconds\n", sleep_time);

    /* always sleep in chunks so we can quit if signalled */
    if (!set->daemon)
        debug(LOW, "Next Poll: ");
    for (i = 0; i < sleep_time; i++) {
        /* check if we've been signalled */
        if (quitting)
            break;
        if (!set->daemon) {
            debug(LOW, "%d...", i + 1);
            fflush(NULL);
        }
        nanosleep(&ts, NULL);
    }
    if (!set->daemon)
        debug(LOW, "\n");
    return (0);
}


/* Timestamp */
void timestamp(char *str) {
   time_t clock;
   struct tm *t;

   clock = time(NULL);
   t = localtime(&clock);
   printf("[%02d/%02d %02d:%02d:%02d %s]\n", t->tm_mon + 1, 
      t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, str);
   return;
}

char *file_timestamp() {
   static char str[BUFSIZE];
   time_t clock;
   struct tm *t;

   clock = time(NULL);
   t = localtime(&clock);
   snprintf(str, sizeof(str), "%02d%02d_%02d:%02d:%02d", t->tm_mon + 1, 
      t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
   return(str);
}

/* Return number double precision seconds difference between timeval structs */
double timediff(struct timeval tv1, struct timeval tv2) {
	double result = 0.0;

	result = ( (double) tv1.tv_usec / MEGA + tv1.tv_sec ) -
		( (double) tv2.tv_usec / MEGA + tv2.tv_sec );

	if (result < 0) {
		result = ( (double) tv2.tv_usec / MEGA + tv2.tv_sec ) -
			( (double) tv1.tv_usec / MEGA + tv1.tv_sec );
	}

	return (result);
}

/* convert a timeval to milliseconds */
unsigned long tv2ms(struct timeval tv) {
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* convert milliseconds to a timeval */
struct timeval ms2tv(unsigned long ms) {
    struct timeval tv;

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    return tv;
}

int checkPID(char *pidfile, config_t *set) {
	FILE *pidptr = NULL;
	pid_t rated_pid;

	rated_pid = getpid();
	if ((pidptr = fopen(pidfile, "r")) != NULL) {
		char temp_pid[BUFSIZE];
		int verify = 0;
		/* rated appears to already be running. */
		while (fgets(temp_pid,BUFSIZE,pidptr)) {
			verify = atoi(temp_pid);
			debug(LOW, "Checking another instance with pid %d.\n", verify);
			if (kill(verify, 0) == 0) {
				fatal("rated is already running. Exiting.\n");
			} else {
				/* This process isn't really running */
				debug(LOW, "PID %d is no longer running. Starting anyway.\n", verify);
				unlink(pidfile);
			}
		}
	}

	/* This is good, rated is not running. */
	if ((pidptr = fopen(pidfile, "w")) != NULL) {
		fprintf(pidptr, "%d\n", rated_pid);
		fclose(pidptr);
		return(0);
	}
	else {
		/* Yuck, we can't even write the PID. Goodbye. */
		return(-1);
	}
}

int alldigits(char *s) {
    int result = TRUE;

	if (*s == '\0') return FALSE;

    while (*s != '\0') {
        if (!(*s >= '0' && *s <= '9')) 
			return FALSE;
        s++;
    }
    return result;
}

/* As per Stevens */
int daemon_init() {
	pid_t pid;
	
	if ( (pid = fork()) < 0)
		return -1;
	else if (pid != 0)
		exit (0);	/* Parent goes buh-bye */
	
	/* child continues */
	setsid();
	/* chdir("/"); */
	umask(0);
	return (0);
}

