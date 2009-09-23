/****************************************************************************
   Program:     $Id: rtgpoll.c,v 1.41 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG SNMP get dumps to MySQL database
****************************************************************************/

#define _REENTRANT
#include "rated.h"
#include "ratedsnmp.h"

/* Yes.  Globals. */
stats_t stats;
char *target_file = NULL;
char *pid_file = PIDFILE;
extern int entries;
host_t *head = NULL;

/* Globals */
config_t config;
config_t *set = &config;

/* for signal handler */
volatile sig_atomic_t waiting;
volatile sig_atomic_t quitting;
volatile sig_atomic_t quit_signal;

char config_paths[CONFIG_PATHS][BUFSIZE];

/* dfp is a debug file pointer.  Points to stderr unless debug=level is set */
FILE *dfp = NULL;


/* Main rated */
int main(int argc, char *argv[]) {
    crew_t crew;
    pthread_t sig_thread;
    sigset_t signal_set;
    struct timeval now;
    unsigned long begin_time, end_time, sleep_time;
    unsigned int poll_time;
    char *conf_file = NULL;
    char errstr[BUFSIZE];
    int ch, i, freed;
    struct timespec ts;
    waiting = FALSE;
    quitting = FALSE;
    quit_signal = 0;

    dfp = stderr;

    /* Check argument count */
    if (argc < 3)
	usage(argv[0]);

    /* Set default environment */
    config_defaults(set);

    /* Parse the command-line. */
    while ((ch = getopt(argc, argv, "c:p:dhmDt:vz")) != EOF)
	switch ((char) ch) {
	case 'c':
	    conf_file = optarg;
	    break;
	case 'd':
	    set->dboff = TRUE;
	    break;
	case 'D':
	    set->daemon = FALSE;
	    break;
	case 'h':
	    usage(argv[0]);
	    break;
	case 'm':
	    set->multiple++;
	    break;
	case 'p':
	    pid_file = optarg;
	    break;
	case 't':
	    target_file = optarg;
	    break;
	case 'v':
	    set->verbose++;
	    break;
	case 'z':
	    set->withzeros = TRUE;
	    break;
	}

    debug(LOW, "rated version %s starting.\n", VERSION);

    if (set->daemon) {
        if (daemon_init() < 0)
            fatal("Could not fork daemon!\n");
        debug(LOW, "Daemon detached\n");
    }

    pthread_mutex_init(&stats.mutex, NULL);

    /* Initialize signal handler */
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGHUP);
    sigaddset(&signal_set, SIGUSR1);
    sigaddset(&signal_set, SIGUSR2);
    sigaddset(&signal_set, SIGTERM);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGQUIT);
    if (!set->multiple) 
        checkPID(pid_file, set);

    if (pthread_sigmask(SIG_BLOCK, &signal_set, NULL) != 0)
        fatal("pthread_sigmask error\n");

    /* Read configuration file to establish local environment */
    if (conf_file) {
        if ((read_rated_config(conf_file, set)) < 0) 
            fatal("Could not read config file: %s\n", conf_file);
    } else {
        conf_file = malloc(BUFSIZE);
        if (!conf_file) 
            fatal("Fatal malloc error!\n");
        for(i=0;i<CONFIG_PATHS;i++) {
            snprintf(conf_file, BUFSIZE, "%s%s", config_paths[i], DEFAULT_CONF_FILE); 
            if (read_rated_config(conf_file, set) >= 0)
                break;
            if (i == CONFIG_PATHS-1) {
                snprintf(conf_file, BUFSIZE, "%s%s", config_paths[0], DEFAULT_CONF_FILE); 
                if ((write_rated_config(conf_file, set)) < 0) 
                    fatal("Couldn't write config file.\n");
            }
        }
    }

    /* these probably aren't thread safe*/
    /* TODO only do this if we're debugging or not daemonised? */
    snmp_enable_stderrlog();
    /* output oids numerically - this is equivalent to -On in the snmp tools */
    netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_OID_OUTPUT_FORMAT, NETSNMP_OID_OUTPUT_NUMERIC);

    debug(LOW, "Initializing threads (%d).\n", set->threads);
    pthread_mutex_init(&(crew.mutex), NULL);
    pthread_cond_init(&(crew.done), NULL);
    pthread_cond_init(&(crew.go), NULL);

    debug(HIGH, "Starting threads...");
    crew.running = set->threads;
    for (i = 0; i < set->threads; i++) {
        crew.member[i].index = i;
        crew.member[i].crew = &crew;
        if (pthread_create(&(crew.member[i].thread), NULL, poller, (void *) &(crew.member[i])) != 0)
	    fatal("pthread_create error\n");
	debug(HIGH, " %i", i);
	}
    debug(HIGH, " done\n");
    if (pthread_create(&sig_thread, NULL, sig_handler, (void *) &(signal_set)) != 0)
	fatal("pthread_create error\n");

    /* spin waiting for all threads to start up */
    debug(HIGH, "Waiting for thread startup.\n");
    ts.tv_sec = 0;
    ts.tv_nsec = 10000000; /* 10 ms */
    gettimeofday(&now, NULL);
    begin_time = tv2ms(now); /* convert to milliseconds */
    while (crew.running > 0 ) {
	nanosleep(&ts, NULL);
    }
    gettimeofday(&now, NULL);
    end_time = tv2ms(now); /* convert to milliseconds */
    debug(HIGH, "Waited %lu milliseconds for thread startup.\n", end_time - begin_time);

    /* build list of hosts to be polled */
    head = hash_target_file(target_file);
    if (head == NULL)
        fatal("Error updating target list.");

    debug(LOW, "rated ready.\n");

    /* Loop Forever Polling Target List */
    while (1) {
	/* check if we've been signalled */
	if (quitting) {
            debug(LOW, "Quitting: received signal %i.\n", quit_signal);
            unlink(pid_file);
            exit(1);
	} else if (waiting) {
            debug(HIGH, "Processing pending SIGHUP.\n");
            /* this just rebuilds the target list
             * so all of the targets will reset to a first poll */
            /* none of the threads should be running at this point so we shouldn't need a lock */
            freed = free_target_list(head);
            debug(HIGH, "Freed %i targets\n", freed);
            head = hash_target_file(target_file);
            waiting = FALSE;
	}

	gettimeofday(&now, NULL);
	begin_time = tv2ms(now);

	PT_MUTEX_LOCK(&(crew.mutex));
        crew.current = head;
	    
	debug(LOW, "Queue ready, broadcasting thread go condition.\n");

	PT_COND_BROAD(&(crew.go));
	PT_MUTEX_UNLOCK(&(crew.mutex));

        /*
         * wait for signals from threads finishing
         * we have to use a do loop because when this starts up the running count will be zero
         * so wait at least once until we get a signal that some thread is done before checking for zero
         */
	PT_MUTEX_LOCK(&(crew.mutex));
        do {
            PT_COND_WAIT(&(crew.done), &(crew.mutex));
        } while (crew.running > 0);
	PT_MUTEX_UNLOCK(&(crew.mutex));

	gettimeofday(&now, NULL);
	end_time = tv2ms(now);
	poll_time = end_time - begin_time;
        /* don't underflow */
        if (poll_time < set->interval) {
	    sleep_time = set->interval - poll_time;
        } else {
            sleep_time = 0;
            stats.slow++;
        }

        /* have to call this before we increment the round counter */
        calc_stats(&stats, poll_time);

        stats.round++;
	debug(LOW, "Poll round %d completed in %u ms.\n", stats.round, poll_time);
	if (set->verbose >= LOW) {
            print_stats(stats, set);
        }

        if (sleep_time > 0) {
            sleepy(sleep_time, set);
        } else {
            debug(LOW, "Slow poll, not sleeping\n");
        }
    } /* while(1) */
    exit(0);
}


/*
 * Signal Handler.  USR1 increases verbosity, USR2 decreases verbosity. 
 * HUP re-reads target list
 */
void *sig_handler(void *arg)
{
    sigset_t *signal_set = (sigset_t *) arg;
    int sig_number;

    while (1) {
	sigwait(signal_set, &sig_number);
	switch (sig_number) {
            case SIGHUP:
                waiting = TRUE;
                break;
            case SIGUSR1:
                set->verbose++;
                break;
            case SIGUSR2:
                set->verbose--;
                break;
            case SIGTERM:
            case SIGINT:
            case SIGQUIT:
                quit_signal = sig_number;
                quitting = TRUE;
                break;
        }
    }
}


void usage(char *prog)
{
    printf("rated v%s\n", VERSION);
    printf("Usage: %s [-dDmz] [-vvv] [-c <file>] -t <file>\n", prog);
    printf("\nOptions:\n");
    printf("  -c <file>   Specify configuration file\n");
    printf("  -D          Don't detach, run in foreground\n");
    printf("  -d          Disable database inserts\n");
    printf("  -t <file>   Specify target file\n");
    printf("  -v          Increase verbosity\n");
    printf("  -m          Allow multiple instances\n");
    printf("  -p <file>   Specify PID file [default /var/run/rated.pid]\n"); 
    printf("  -z          Database zero delta inserts\n");
    printf("  -h          Help\n");
    exit(-1);
}
