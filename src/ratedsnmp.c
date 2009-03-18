/****************************************************************************
   Program:     $Id: rtgsnmp.c,v 1.41 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG SNMP Routines
****************************************************************************/

#include "common.h"
#include "rated.h"
#include "rateddbi.h"

#ifdef OLD_UCD_SNMP
 #include "asn1.h"
 #include "snmp_api.h"
 #include "snmp_impl.h"
 #include "snmp_client.h"
 #include "mib.h"
 #include "snmp.h"
#else
 #include "net-snmp-config.h"
 #include "net-snmp-includes.h"
#endif

extern target_t *current;
extern stats_t stats;
extern config_t *set;

void cancel_lock(void *arg)
{
    /* to be called from the cleanup handler */
    PT_MUTEX_UNLOCK(arg);
}

void cleanup_db(void *arg)
{
    db_disconnect();
}

void *poller(void *thread_args)
{
    worker_t *worker = (worker_t *) thread_args;
    crew_t *crew = worker->crew;
    target_t *entry = NULL;
    void *sessp = NULL;
    struct snmp_session session;
    struct snmp_pdu *pdu = NULL;
    struct snmp_pdu *response = NULL;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    struct variable_list *vars = NULL;
    unsigned long long result = 0;
    unsigned long long last_value = 0;
    unsigned long long insert_val = 0;
    int poll_status = 0, init = 0;
    int oid_len = 0;
    int bits = 0;
    char *storedoid;
    char *result_string;
    int cur_work = 0;
    int prev_work = 99999999;
    int loop_count = 0;
    double rate = 0;
    struct timezone tzp;
    struct timeval current_time;
    struct timeval last_time;
    int db_reconnect = FALSE;
    int db_error = FALSE;

    /* for thread settings */
    int oldstate, oldtype;

    tdebug(HIGH, "starting.\n");

    pthread_cleanup_push(cleanup_db, NULL);

    if (!(set->dboff)) {
	/* load the database driver */
	if (!(db_init(set)))
            fatal("** Database error - check configuration.\n");
	/* connect to the database */
	if (!(db_connect(set)))
            fatal("server not responding.\n");
        /* set up cancel function for exit */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    }

    PT_MUTEX_LOCK(&crew->mutex);
    tdebug(HIGH, "running.\n");
    crew->running++;
    PT_MUTEX_UNLOCK(&crew->mutex);

    while (1) {

        /* reset variables */
	result = insert_val = 0;
	rate = 0;
	anOID_len = MAX_OID_LEN;

/*
        if(loop_count >= POLLS_PER_TRANSACTION) {
            tdebug(HIGH, "doing commit on %d\n", POLLS_PER_TRANSACTION);
            db_status = db_commit(); 
            loop_count = 0;
        }
*/

	/* see if we've been cancelled before we start another loop */
	pthread_testcancel();

	tdebug(DEVELOP, "locking (wait on work)\n");
	PT_MUTEX_LOCK(&crew->mutex);
	/* add an unlock to the cancel stack */
	pthread_cleanup_push(cancel_lock, &crew->mutex);	

	/* TODO crew->running-- if we're cancelled */

	while (current == NULL) {
            PT_COND_WAIT(&crew->go, &crew->mutex);
	}
	tdebug(DEVELOP, "done waiting, received go (work cnt: %d)\n", crew->work_count);
/*
        cur_work = crew->work_count;
        if(cur_work > prev_work) {
            tdebug(HIGH, "doing commit at %d\n", time(NULL));
            db_status = db_commit(); 
            loop_count = 0;
        }
        prev_work = cur_work;
*/

	if (current != NULL) {
            tdebug(DEVELOP, "processing %s@%s (%d work units remain in queue)\n",
                current->objoid, current->host->host, crew->work_count);
	    /* TODO only do this if we're debugging or not daemonised? */
	    snmp_enable_stderrlog();
            snmp_sess_init(&session);

            if (current->host->snmp_ver == 2)
                session.version = SNMP_VERSION_2c;
            else
                session.version = SNMP_VERSION_1;

            session.peername = current->host->host;
            session.community = current->host->community;
            /* TODO move this into struct so we're not calculating it every time */
            session.community_len = strlen(session.community);
            session.remote_port = set->snmp_port;

	    sessp = snmp_sess_open(&session);

            /* TODO check return status */
	    read_objid(current->objoid, anOID, &anOID_len);

	    entry = current;
	    last_value = current->last_value;
	    /* save the time so we can calculate rate */
	    last_time = current->last_time;
	    init = current->init;
	    bits = current->bits;

            oid_len = strlen(current->objoid);
            /* TODO check malloc return status */
            storedoid = (char*)malloc(oid_len + 1);
	    strncpy(storedoid, current->objoid, oid_len + 1);

            current = getNext();
	}

	tdebug(DEVELOP, "unlocking (done grabbing current)\n");
	PT_MUTEX_UNLOCK(&crew->mutex);

	/* take the unlock off the cancel stack */
	pthread_cleanup_pop(FALSE);

        pdu = snmp_pdu_create(SNMP_MSG_GET);
	snmp_add_null_var(pdu, anOID, anOID_len);
	if (sessp != NULL) 
            poll_status = snmp_sess_synch_response(sessp, pdu, &response);
	else
            poll_status = STAT_DESCRIP_ERROR;

	/* Collect response and process stats */
	PT_MUTEX_LOCK(&stats.mutex);
	if (poll_status == STAT_DESCRIP_ERROR) {
	    stats.errors++;
            tdebug(LOW, "*** SNMP Error: (%s) Bad descriptor.\n", session.peername);
	} else if (poll_status == STAT_TIMEOUT) {
	    stats.no_resp++;
	    tdebug(LOW, "*** SNMP No response: (%s@%s).\n", storedoid, session.peername);
	} else if (poll_status != STAT_SUCCESS) {
	    stats.errors++;
	    tdebug(LOW, "*** SNMP Error: (%s@%s) Unsuccessuful (%d).\n", storedoid, session.peername, poll_status);
	} else if (poll_status == STAT_SUCCESS && response->errstat != SNMP_ERR_NOERROR) {
	    stats.errors++;
	    tdebug(LOW, "*** SNMP Error: (%s@%s) %s\n", storedoid, session.peername, snmp_errstring(response->errstat));
	} else if (poll_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR && response->variables->type == SNMP_NOSUCHINSTANCE) {
	    stats.errors++;
	    tdebug(LOW, "*** SNMP Error: No Such Instance Exists (%s@%s)\n", storedoid, session.peername);
	} else if (poll_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
	    stats.polls++;
	}
	PT_MUTEX_UNLOCK(&stats.mutex);

	/*
	 * Liftoff, successful poll, process it
	 */
	/* Get the current time */
	gettimeofday(&current_time, &tzp);

	if (poll_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR && response->variables->type != SNMP_NOSUCHINSTANCE) {
	    vars = response->variables;
            if (set->verbose >= DEBUG) {
                /* we only do this if we're printing out debug, so don't allocate memory unless we need it */
                /* this seems like a waste but it's difficult to predict the length of the result string
                 * maybe use sprint_realloc_value but it's a PITA */
                /* TODO check return value */
                result_string = (char*)malloc(BUFSIZE);
#ifdef OLD_UCD_SNMP
                sprint_value(result_string, anOID, anOID_len, vars);
#else
                /* this results in something like 'Counter64: 11362777584380' */
                /* TODO check return value */
                snprint_value(result_string, BUFSIZE, anOID, anOID_len, vars);
#endif
                /* don't use tdebug because there's a signal race between when we malloc the memory and here */
                tdebug_all("(%s@%s) %s\n", storedoid, session.peername, result_string);
                free(result_string);
            }
	    switch (vars->type) {
		/*
		 * Switch over vars->type and modify/assign result accordingly.
		 */
		case ASN_COUNTER64:
		    result = vars->val.counter64->high;
		    result = result << 32;
		    result = result + vars->val.counter64->low;
		    break;
		case ASN_COUNTER:
		    result = (unsigned long) *(vars->val.integer);
		    break;
		case ASN_INTEGER:
		    result = (unsigned long) *(vars->val.integer);
		    break;
		case ASN_GAUGE:
		    result = (unsigned long) *(vars->val.integer);
		    break;
		case ASN_TIMETICKS:
		    result = (unsigned long) *(vars->val.integer);
		    break;
		case ASN_OPAQUE:
		    result = (unsigned long) *(vars->val.integer);
		    break;
		case ASN_OCTET_STR:
                /* TODO should we handle negative numbers? */
#ifdef HAVE_STRTOULL
		    result = strtoull(vars->val.string, NULL, 0);
                    if (result == ULLONG_MAX && errno == ERANGE) {
#else
		    result = strtoul(vars->val.string, NULL, 0);
                    if (result == ULONG_MAX && errno == ERANGE) {
#endif
                        tdebug(LOW, "Negative number found: %s\n", vars->val.string);
                        goto cleanup;
                    }
		    break;
		default:
		    /* no result that we can use, restart the polling loop */
		    /* TODO should we remove this target from the list? */
		    goto cleanup;
	    }

            /* zero delta */
            if (result == last_value) {
                tdebug(DEBUG, "zero delta: %llu = %llu\n", result, last_value);
                PT_MUTEX_LOCK(&stats.mutex);
                stats.zero++;
                PT_MUTEX_UNLOCK(&stats.mutex);
                if (set->withzeros) {
                    insert_val = result;
                } else {
                    goto cleanup;
                }
            /* gauges */
            } else if (bits == 0) {
                insert_val = result;
            /* treat all other values as counters */
            /* Counter Wrap Condition */
            } else if (result < last_value) {
                PT_MUTEX_LOCK(&stats.mutex);
                stats.wraps++;
                PT_MUTEX_UNLOCK(&stats.mutex);

                if (bits == 32) insert_val = (THIRTYTWO - last_value) + result;
                else if (bits == 64) insert_val = (SIXTYFOUR - last_value) + result;

                rate = insert_val / timediff(current_time, last_time);

                tdebug(LOW, "*** Counter Wrap (%s@%s) [poll: %llu][last: %llu][insert: %llu]\n",
                    storedoid, session.peername, result, last_value, insert_val);
            /* Not a counter wrap and this is not the first poll */
            } else if ((last_value >= 0) && (init != NEW)) {
                insert_val = result - last_value;
                rate = insert_val / timediff(current_time, last_time);
            /* last_value < 0, so this must be the first poll */
            } else {
                tdebug(HIGH, "First Poll, Normalizing\n");
                goto cleanup;
            }

            /* Check for bogus data, either negative or unrealistic */
            if (insert_val > entry->maxspeed || result < 0) {
                tdebug(LOW, "*** Out of Range (%s@%s) [insert_val: %llu] [oor: %lld]\n",
                    storedoid, session.peername, insert_val, entry->maxspeed);
                PT_MUTEX_LOCK(&stats.mutex);
                stats.out_of_range++;
                PT_MUTEX_UNLOCK(&stats.mutex);
                goto cleanup;
            }

            if (rate) tdebug(DEBUG, "(%lld - %lld = %llu) / %.15f = %.15f\n", result, last_value, insert_val, timediff(current_time, last_time), rate);

            /* TODO do we need to check for zero values again? */

            if (!(set->dboff)) {
                if (db_reconnect) {
                    /* try and reconnect */
                    if (db_connect(set))
                        db_reconnect = FALSE;
                    else
                        goto cleanup;
                }
                if ( (insert_val > 0) || (set->withzeros) ) {
                    tdebug(DEVELOP, "db_insert sent: %s %d %llu %.15f\n", entry->table, entry->iid, insert_val, rate);
                    /* insert into the database */
                    if (db_insert(entry->table, entry->iid, insert_val, rate)) {
                        PT_MUTEX_LOCK(&stats.mutex);
                        stats.db_inserts++;
                        PT_MUTEX_UNLOCK(&stats.mutex);
                        db_error = FALSE;
                    } else {
                        PT_MUTEX_LOCK(&stats.mutex);
                        stats.db_errors++;
                        PT_MUTEX_UNLOCK(&stats.mutex);
                        /* try and reconnect on the next poll */
                        if (!db_status()) {
                            /* first disconnect to close the handle */
                            db_disconnect();
                            db_reconnect = TRUE;
                        } else {
                            /* we have a db connection but got some other error */
                            db_error = TRUE;
                        }
                    }
                } /* zero */
            } /* !dboff */
        } /* STAT_SUCCESS */

/*
        tdebug(HIGH, "doing commit\n");
        db_status = db_commit();
*/

cleanup:
        if (sessp != NULL) {
            snmp_sess_close(sessp);
            if (response != NULL) snmp_free_pdu(response);
        }

        free(storedoid);

        tdebug(DEVELOP, "locking (update work_count)\n");
        PT_MUTEX_LOCK(&crew->mutex);
        crew->work_count--;

        /* update the time if we inserted ok */
        if (!(db_reconnect || db_error)) {
            entry->last_time = current_time;	
            /* Only if we received a positive result back do we update the last_value object */
            if (poll_status == STAT_SUCCESS) {
                entry->last_value = result;
                if (init == NEW) entry->init = LIVE;
            }
        }

        if (crew->work_count <= 0) {
            tdebug(HIGH, "Queue processed. Broadcasting thread done condition.\n");
            PT_COND_BROAD(&crew->done);
        }
        tdebug(DEVELOP, "unlocking (update work_count)\n");

        PT_MUTEX_UNLOCK(&crew->mutex);

        loop_count++;
    } /* while(1) */
    pthread_cleanup_pop(FALSE);
/* Not reached */
}
