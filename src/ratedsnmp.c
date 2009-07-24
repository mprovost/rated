/****************************************************************************
   Program:     $Id: rtgsnmp.c,v 1.41 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG SNMP Routines
****************************************************************************/

#include "common.h"
#include "rated.h"
#include "ratedsnmp.h"
#include "rateddbi.h"

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
    host_t *host = NULL;
    target_t *head;
    target_t *entry = NULL;
    void *sessp;
    struct snmp_pdu *pdu = NULL;
    struct snmp_pdu *response = NULL;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    struct variable_list *vars = NULL;
    unsigned long long result = 0;
    unsigned long long insert_val = 0;
    int poll_status = 0;
    char *result_string;
    int cur_work = 0;
    int prev_work = 99999999;
    int loop_count = 0;
    double rate = 0;
    struct timezone tzp;
    struct timeval current_time;
    struct timeval last_time;
    /* this forces the db to connect on the first poll (that we have something to insert) */
    int db_reconnect = TRUE;
    int db_error = FALSE;

    /* for thread settings */
    int oldstate, oldtype;

    tdebug(HIGH, "starting.\n");

    pthread_cleanup_push(cleanup_db, NULL);

    if (!(set->dboff)) {
	/* load the database driver */
	if (!(db_init(set)))
            fatal("** Database error - check configuration.\n");
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
        db_error = FALSE;
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

        /* wait for current to not be NULL */
	while (crew->current == NULL) {
            PT_COND_WAIT(&crew->go, &crew->mutex);
	}
	tdebug(DEVELOP, "done waiting, received go\n");
/*
        cur_work = crew->work_count;
        if(cur_work > prev_work) {
            tdebug(HIGH, "doing commit at %d\n", time(NULL));
            db_status = db_commit(); 
            loop_count = 0;
        }
        prev_work = cur_work;
*/

	if (crew->current) {
            host = crew->current;
            crew->current = crew->current->next;
        } /* TODO else */

        /* we have a host, release the lock */
	tdebug(DEVELOP, "unlocking (done grabbing current = %s)\n", host->host);
	PT_MUTEX_UNLOCK(&crew->mutex);

        /* take the unlock off the cancel stack */
        pthread_cleanup_pop(FALSE);

        head = host->current;

        /* open an snmp session once for all targets for this host for this round */
        sessp = snmp_sess_open(&host->session);

        if (sessp == NULL) {
            /* skip to next host */
            continue;
        }

        /* loop through the targets for this host */
        while (host->current) {
            entry = host->current;

            tdebug(DEVELOP, "processing %s@%s\n",
                entry->objoid, host->host);

            /* save the time so we can calculate rate */
            last_time = entry->last_time;

            pdu = snmp_pdu_create(SNMP_MSG_GET);
            /* TODO check return status */
            read_objid(entry->objoid, anOID, &anOID_len);
            snmp_add_null_var(pdu, anOID, anOID_len);
            /* this will free the pdu on error so we can't save them for reuse between rounds */
            poll_status = snmp_sess_synch_response(sessp, pdu, &response);

            /* Collect response and process stats */
            PT_MUTEX_LOCK(&stats.mutex);
            if (poll_status == STAT_DESCRIP_ERROR) {
                stats.errors++;
                tdebug(LOW, "*** SNMP Error: (%s) Bad descriptor.\n", host->session.peername);
            } else if (poll_status == STAT_TIMEOUT) {
                stats.no_resp++;
                tdebug(LOW, "*** SNMP No response: (%s@%s).\n", entry->objoid, host->session.peername);
            } else if (poll_status != STAT_SUCCESS) {
                stats.errors++;
                tdebug(LOW, "*** SNMP Error: (%s@%s) Unsuccessful (%d).\n", entry->objoid, host->session.peername, poll_status);
            } else if (poll_status == STAT_SUCCESS && response->errstat != SNMP_ERR_NOERROR) {
                stats.errors++;
                tdebug(LOW, "*** SNMP Error: (%s@%s) %s\n", entry->objoid, host->session.peername, snmp_errstring(response->errstat));
            } else if (poll_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR && response->variables->type == SNMP_NOSUCHINSTANCE) {
                stats.errors++;
                tdebug(LOW, "*** SNMP Error: No Such Instance Exists (%s@%s)\n", entry->objoid, host->session.peername);
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
                    tdebug_all("(%s@%s) %s\n", entry->objoid, host->session.peername, result_string);
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

                tdebug(DEBUG, "result = %llu, last_value = %llu, bits = %hi, init = %i\n", result, entry->last_value, entry->bits, entry->init);
                /* zero delta */
                if (result == entry->last_value) {
                    tdebug(DEBUG, "zero delta: %llu = %llu\n", result, entry->last_value);
                    PT_MUTEX_LOCK(&stats.mutex);
                    stats.zero++;
                    PT_MUTEX_UNLOCK(&stats.mutex);
                    if (set->withzeros) {
                        insert_val = result;
                    } else {
                        goto cleanup;
                    }
                /* gauges */
                } else if (entry->bits == 0) {
                    insert_val = result;
                /* treat all other values as counters */
                /* Counter Wrap Condition */
                } else if (result < entry->last_value) {
                    PT_MUTEX_LOCK(&stats.mutex);
                    stats.wraps++;
                    PT_MUTEX_UNLOCK(&stats.mutex);

                    if (entry->bits == 32) insert_val = (THIRTYTWO - entry->last_value) + result;
                    else if (entry->bits == 64) insert_val = (SIXTYFOUR - entry->last_value) + result;

                    rate = insert_val / timediff(current_time, last_time);

                    tdebug(LOW, "*** Counter Wrap (%s@%s) [poll: %llu][last: %llu][insert: %llu]\n",
                        entry->objoid, host->session.peername, result, entry->last_value, insert_val);
                /* Not a counter wrap and this is not the first poll */
                } else if ((entry->last_value >= 0) && (entry->init != NEW)) {
                    insert_val = result - entry->last_value;
                    rate = insert_val / timediff(current_time, last_time);
                /* entry->last_value < 0, so this must be the first poll */
                } else {
                    tdebug(HIGH, "First Poll, Normalizing\n");
                    goto cleanup;
                }

                if (rate) tdebug(DEBUG, "(%lld - %lld = %llu) / %.15f = %.15f\n", result, entry->last_value, insert_val, timediff(current_time, last_time), rate);

                /* TODO do we need to check for zero values again? */

                if (!(set->dboff)) {
                    if ( (insert_val > 0) || (set->withzeros) ) {
                        if (db_reconnect) {
                            /* try and (re)connect */
                            if (db_connect(set)) {
                                db_reconnect = FALSE;
                            } else {
                                /* the db driver will print an error itself */
                                db_error = TRUE;
                                PT_MUTEX_LOCK(&stats.mutex);
                                stats.db_errors++;
                                PT_MUTEX_UNLOCK(&stats.mutex);
                                goto cleanup;
                            }
                        }
                        tdebug(DEVELOP, "db_insert sent: %s %d %llu %.15f\n", entry->table, entry->iid, insert_val, rate);
                        /* insert into the database */
                        if (db_insert(entry->table, entry->iid, insert_val, rate)) {
                            PT_MUTEX_LOCK(&stats.mutex);
                            stats.db_inserts++;
                            PT_MUTEX_UNLOCK(&stats.mutex);
                        } else {
                            db_error = TRUE;
                            PT_MUTEX_LOCK(&stats.mutex);
                            stats.db_errors++;
                            PT_MUTEX_UNLOCK(&stats.mutex);
                            if (!db_status()) {
                                /* first disconnect to close the handle */
                                db_disconnect();
                                /* try and reconnect on the next poll */
                                db_reconnect = TRUE;
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
                if (response != NULL) snmp_free_pdu(response);
            }

            /* update the time if we inserted ok */
            if (!db_error) {
                entry->last_time = current_time;	
                /* Only if we received a positive result back do we update the entry->last_value object */
                if (poll_status == STAT_SUCCESS) {
                    entry->last_value = result;
                    if (entry->init == NEW) entry->init = LIVE;
                } else {
                    tdebug(DEBUG, "no success!\n");
                }
            } else {
                tdebug(DEBUG, "db_reconnect = %i db_error = %i!\n", db_reconnect, db_error);
            }

            /* loop_count++; */
            /* move to next target */
            host->current = host->current->next;
        } /* while (host->current) */
        if (sessp != NULL)
            snmp_sess_close(sessp);
        /* reset back to start */
        host->current = head;
        /* done with targets for this host, check if it was the last host */
        /* but other threads may still be doing their target lists... */
        if (host->next == NULL) {
            tdebug(HIGH, "Queue processed. Broadcasting thread done condition.\n");
            /* this will wake up the main thread which will start sleeping for the next round */
            PT_COND_BROAD(&crew->done);
        }
    } /* while(1) */
    pthread_cleanup_pop(FALSE);
/* Not reached */
}
