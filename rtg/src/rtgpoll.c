/****************************************************************************
   Program:     $Id: rtgpoll.c,v 1.41 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG SNMP get dumps to MySQL database
****************************************************************************/

#define _REENTRANT
#include "rtg.h"

/* Yes.  Globals. */
stats_t stats =
{PTHREAD_MUTEX_INITIALIZER, 0, 0, 0, 0, 0, 0, 0, 0, 0.0};
char *target_file = NULL;
target_t *current = NULL;
char *pid_file = PIDFILE;
int entries = 0;

/* Globals */
config_t config;
config_t *set = &config;
int lock;
int waiting;
char config_paths[CONFIG_PATHS][BUFSIZE];

/* dfp is a debug file pointer.  Points to stderr unless debug=level is set */
FILE *dfp = NULL;


/* Main rtgpoll */
int main(int argc, char *argv[]) {
    crew_t crew;
    pthread_t sig_thread;
    sigset_t signal_set;
    struct timeval now;
    double begin_time, end_time, sleep_time;
    char *conf_file = NULL;
    char errstr[BUFSIZE];
    int ch, i;

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

	debug(LOW, "RTG version %s starting.\n", VERSION);

	if (set->daemon) {
		if (daemon_init() < 0)
			fatal("Could not fork daemon!\n");
		debug(LOW, "Daemon detached\n");
	}


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
		printf("pthread_sigmask error\n");

    /* Read configuration file to establish local environment */
    if (conf_file) {
      if ((read_rtg_config(conf_file, set)) < 0) 
		fatal("Could not read config file: %s\n", conf_file);
    } else {
      conf_file = malloc(BUFSIZE);
      if (!conf_file) 
         fatal("Fatal malloc error!\n");
      for(i=0;i<CONFIG_PATHS;i++) {
        snprintf(conf_file, BUFSIZE, "%s%s", config_paths[i], DEFAULT_CONF_FILE); 
        if (read_rtg_config(conf_file, set) >= 0)
           break;
        if (i == CONFIG_PATHS-1) {
			snprintf(conf_file, BUFSIZE, "%s%s", config_paths[0], DEFAULT_CONF_FILE); 
			if ((write_rtg_config(conf_file, set)) < 0) 
				fatal("Couldn't write config file.\n");
        }
      }
    }


    /* hash list of targets to be polled */
    init_hash();
	entries = hash_target_file(target_file);
    if (entries <= 0) 
		fatal("Error updating target list.");

	debug(LOW, "Initializing threads (%d).\n", set->threads);
    pthread_mutex_init(&(crew.mutex), NULL);
    pthread_cond_init(&(crew.done), NULL);
    pthread_cond_init(&(crew.go), NULL);
    crew.work_count = 0;

    /* Initialize the SNMP session */
	debug(LOW, "Initializing SNMP (port %d).\n", set->snmp_port);
    init_snmp("RTG");

	debug(HIGH, "\nStarting threads.\n");
    for (i = 0; i < set->threads; i++) {
		crew.member[i].index = i;
		crew.member[i].crew = &crew;
		if (pthread_create(&(crew.member[i].thread), NULL, poller, (void *) &(crew.member[i])) != 0)
			printf("pthread_create error\n");
	}
    if (pthread_create(&sig_thread, NULL, sig_handler, (void *) &(signal_set)) != 0)
		printf("pthread_create error\n");

    /* give threads time to start up */
    sleep(1);

	debug(LOW, "RTG Ready.\n");

    /* Loop Forever Polling Target List */
    while (1) {
	lock = TRUE;
	gettimeofday(&now, NULL);
	begin_time = (double) now.tv_usec / 1000000 + now.tv_sec;

	PT_MUTEX_LOCK(&(crew.mutex));
	init_hash_walk();
	current = getNext();
	crew.work_count = entries;
	PT_MUTEX_UNLOCK(&(crew.mutex));
	    
	if (set->verbose >= LOW) {
		if (set->daemon)
			sysloginfo("Queue ready, broadcasting thread go condition.");
		else
        	timestamp("Queue ready, broadcasting thread go condition.");
	}
	PT_COND_BROAD(&(crew.go));
	PT_MUTEX_LOCK(&(crew.mutex));
	    
	while (crew.work_count > 0) {
		PT_COND_WAIT(&(crew.done), &(crew.mutex));
	}
	PT_MUTEX_UNLOCK(&(crew.mutex));

	gettimeofday(&now, NULL);
	lock = FALSE;
	end_time = (double) now.tv_usec / 1000000 + now.tv_sec;
	stats.poll_time = end_time - begin_time;
        stats.round++;
	sleep_time = set->interval - stats.poll_time;

	if (waiting) {
		debug(HIGH, "Processing pending SIGHUP.\n");
	    entries = hash_target_file(target_file);
	    waiting = FALSE;
	}

	if (set->verbose >= LOW) {
        snprintf(errstr, sizeof(errstr), "Poll round %d complete.", stats.round);
		if (set->daemon)
			sysloginfo(errstr);
		else
			timestamp(errstr);
	    print_stats(stats, set);
    }

	if (sleep_time <= 0)
	    stats.slow++;
	else
	    sleepy(sleep_time, set);
    } /* while */

    exit(0);
}


/* Signal Handler.  USR1 increases verbosity, USR2 decreases verbosity. 
   HUP re-reads target list */
void *sig_handler(void *arg)
{
    sigset_t *signal_set = (sigset_t *) arg;
    int sig_number;

    while (1) {
	sigwait(signal_set, &sig_number);
	switch (sig_number) {
            case SIGHUP:
                if(lock) {
                    waiting = TRUE;
                }
                else {
                    entries = hash_target_file(target_file);
                    waiting = FALSE;
                }
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
		debug(LOW, "Quiting: received signal %d.\n", sig_number);
                unlink(pid_file);
                exit(1);
                break;
        }
   }
}


void usage(char *prog)
{
    printf("rtgpoll - RTG v%s\n", VERSION);
    printf("Usage: %s [-dDmz] [-vvv] [-c <file>] -t <file>\n", prog);
    printf("\nOptions:\n");
    printf("  -c <file>   Specify configuration file\n");
    printf("  -D          Don't detach, run in foreground\n");
    printf("  -d          Disable database inserts\n");
    printf("  -t <file>   Specify target file\n");
    printf("  -v          Increase verbosity\n");
	printf("  -m          Allow multiple instances\n");
    printf("  -p <file>	  Specify PID file [default /var/run/rtgpoll.pid]\n"); 
	printf("  -z          Database zero delta inserts\n");
    printf("  -h          Help\n");
    exit(-1);
}
