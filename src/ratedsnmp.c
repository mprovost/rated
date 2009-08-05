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

int snmp_getnext(void *sessp, oid *anOID, size_t *anOID_len, unsigned long long *result, struct timeval *current_time) {
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    netsnmp_variable_list *vars;
    netsnmp_session *session;
    int getnext_status = 0;
    int return_status = 0;
    char *oid_string;
    char *result_string;

    pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
    snmp_add_null_var(pdu, anOID, *anOID_len);

    oid_string = (char*)malloc(SPRINT_MAX_LEN);
    /* TODO check return value */
    snprint_objid(oid_string, SPRINT_MAX_LEN, anOID, *anOID_len);
    session = snmp_sess_session(sessp);
    debug(LOW, "Polling (%s@%s)\n", oid_string, session->peername);

    /* this will free the pdu on error so we can't save them for reuse between rounds */
    getnext_status = snmp_sess_synch_response(sessp, pdu, &response);

    /* Get the current time */
    gettimeofday(current_time, NULL);

    /* Collect response and process stats */
    PT_MUTEX_LOCK(&stats.mutex);
    if (getnext_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        vars = response->variables;
        return_status = 1; /* success */

        stats.polls++;
        PT_MUTEX_UNLOCK(&stats.mutex);

        if (set->verbose >= DEBUG) {
            /* convert the opaque session pointer back into a session struct for debug output */
            session = snmp_sess_session(sessp);

            /* we only do this if we're printing out debug, so don't allocate memory unless we need it */
            /* this seems like a waste but it's difficult to predict the length of the result string
             * maybe use sprint_realloc_value but it's a PITA */
            result_string = (char*)malloc(SPRINT_MAX_LEN);
            /* this results in something like 'Counter64: 11362777584380' */
            /* TODO check return value */
            snprint_value(result_string, SPRINT_MAX_LEN, vars->name, vars->name_length, vars);
            oid_string = (char*)malloc(SPRINT_MAX_LEN);
            /* TODO check return value */
            snprint_objid(oid_string, SPRINT_MAX_LEN, vars->name, vars->name_length);
            /* don't use tdebug because there's a signal race between when we malloc the memory and here */
            debug_all("(%s@%s) %s\n", oid_string, session->peername, result_string);

            free(result_string);
            free(oid_string);
        }
        switch (vars->type) {
            /*
             * Switch over vars->type and modify/assign result accordingly.
             */
            case ASN_COUNTER64:
                *result = vars->val.counter64->high;
                *result = *result << 32;
                *result = *result + vars->val.counter64->low;
                break;
            case ASN_COUNTER:
                *result = (unsigned long) *(vars->val.integer);
                break;
            case ASN_INTEGER:
                *result = (unsigned long) *(vars->val.integer);
                break;
            case ASN_GAUGE:
                *result = (unsigned long) *(vars->val.integer);
                break;
            case ASN_TIMETICKS:
                *result = (unsigned long) *(vars->val.integer);
                break;
            case ASN_OPAQUE:
                *result = (unsigned long) *(vars->val.integer);
                break;
            case ASN_OCTET_STR:
            /* TODO should we handle negative numbers? */
#ifdef HAVE_STRTOULL
                *result = strtoull(vars->val.string, NULL, 0);
                if (*result == ULLONG_MAX && errno == ERANGE) {
#else
                *result = strtoul(vars->val.string, NULL, 0);
                if (*result == ULONG_MAX && errno == ERANGE) {
#endif
                    debug(LOW, "Negative number found: %s\n", vars->val.string);
                    return_status = -1;
                }
                break;
            default:
                /* no result that we can use, restart the polling loop */
                /* TODO should we remove this target from the list? */
                *result = 0;
                return_status = 0;
        } /* switch (vars->type) */
        /* copy the next oid so we can be called again */
        oid_string = (char*)malloc(SPRINT_MAX_LEN);
        snprint_objid(oid_string, SPRINT_MAX_LEN, vars->name, vars->name_length);
        debug(LOW, "Setting next: (%s@%s)\n", oid_string, session->peername);
        memmove(anOID, vars->name, vars->name_length * sizeof(oid));
        *anOID_len = vars->name_length;
        free(oid_string);
    } else { 
        return_status = 0; /* error */
        /* convert the opaque session pointer back into a session struct for debug output */
        session = snmp_sess_session(sessp);
        oid_string = (char*)malloc(SPRINT_MAX_LEN);
        snprint_objid(oid_string, SPRINT_MAX_LEN, anOID, *anOID_len);

        switch (getnext_status) {
            case STAT_DESCRIP_ERROR:
                stats.errors++;
                debug(LOW, "*** SNMP Error: (%s) Bad descriptor.\n", session->peername);
                break;
            case STAT_TIMEOUT:
                stats.no_resp++;
                debug(LOW, "*** SNMP No response: (%s@%s).\n", oid_string, session->peername);
                break;
            case STAT_SUCCESS:
                stats.errors++;
                if (response->variables->type == SNMP_NOSUCHINSTANCE) {
                    debug(LOW, "*** SNMP Error: No Such Instance Exists (%s@%s)\n", oid_string, session->peername);
                } else {
                    debug(LOW, "*** SNMP Error: (%s@%s) %s\n", oid_string, session->peername, snmp_errstring(response->errstat));
                }
                break;
            default:
                stats.errors++;
                debug(LOW, "*** SNMP Error: (%s@%s) Unsuccessful (%i).\n", oid_string, session->peername, getnext_status);
                break;
        }
        PT_MUTEX_UNLOCK(&stats.mutex);
        free(oid_string);

        anOID = NULL;
        anOID_len = 0;
    }

    if (response)
        snmp_free_pdu(response);
    return return_status;
}

void *poller(void *thread_args)
{
    worker_t *worker = (worker_t *) thread_args;
    crew_t *crew = worker->crew;
    host_t *host = NULL;
    target_t *head;
    target_t *entry = NULL;
    netsnmp_session *sessp;
    int getnext_status = 0;
    unsigned int getnexts;
    unsigned long long result = 0;
    unsigned long long insert_val = 0;
    /*
    int cur_work = 0;
    int prev_work = 99999999;
    int loop_count = 0;
    */
    double rate = 0;
    struct timeval current_time;
    struct timeval last_time;
    struct timeval now;
    double begin_time, end_time;
    /* this forces the db to connect on the first poll (that we have something to insert) */
    int db_reconnect = TRUE;
    int db_error = FALSE;
    oid *anOID;
    size_t *anOID_len;

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

    anOID = malloc(MAX_OID_LEN * sizeof(oid));
    anOID_len = malloc(sizeof(size_t));

    while (1) {

        /* reset variables */
	result = insert_val = 0;
	rate = 0;
        db_error = FALSE;

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

        /* TODO do we need this check at all? */
	if (crew->current) {
            host = crew->current;
            crew->current = crew->current->next;
        } /* TODO else */

        /* we have a host, release the lock */
	tdebug(DEVELOP, "unlocking (done grabbing current = %s)\n", host->host);
	PT_MUTEX_UNLOCK(&crew->mutex);

        /* take the unlock off the cancel stack */
        pthread_cleanup_pop(FALSE);

        /* TODO get rid of this it's the same as entry */
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

            tdebug(DEVELOP, "processing %s@%s\n", entry->objoid, host->host);

            memmove(anOID, entry->anOID, entry->anOID_len * sizeof(oid));
            *anOID_len = head->anOID_len;

            /* save the time so we can calculate rate */
            last_time = entry->last_time;

            getnexts = 0;

            gettimeofday(&now, NULL);
            begin_time = now.tv_sec * 1000 + now.tv_usec / 1000; /* convert to milliseconds */

            while (anOID) {
                if (*anOID_len < entry->anOID_len) {
                    tdebug(DEBUG, "snmpgetnext done <\n");
                    break;
                } else if ((memcmp(&entry->anOID, anOID, entry->anOID_len * sizeof(oid)) != 0)) {
                    print_objid(entry->anOID, entry->anOID_len);
                    print_objid(anOID, *anOID_len);
                    tdebug(DEBUG, "snmpgetnext done memcmp\n");
                    break;
                }
                /* do the actual snmp poll */
                getnext_status = snmp_getnext(sessp, anOID, anOID_len, &result, &current_time);

                if (!getnext_status) {
                    /* skip to next target */
                    //host->current = host->current->next;
                    tdebug(DEBUG, "!getnext_status\n");
                    break;
                }

                getnexts++;


                tdebug(DEBUG, "result = %llu, last_value = %llu, bits = %hi, init = %i\n", result, entry->last_value, entry->bits, entry->init);

                /* zero delta */
                /* TODO zero result on first poll */
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
                /*
                 * insert into the db
                 */
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

        /*
                tdebug(HIGH, "doing commit\n");
                db_status = db_commit();
        */

cleanup:
                /* update the time if we inserted ok */
                if (!db_error) {
                    entry->last_time = current_time;	
                    /* Only if we received a positive result back do we update the entry->last_value object */
                    if (getnext_status) {
                        entry->last_value = result;
                        if (entry->init == NEW) entry->init = LIVE;
                    } else {
                        tdebug(DEBUG, "no success:%i!\n", getnext_status);
                    }
                } else {
                    tdebug(DEBUG, "db_reconnect = %i db_error = %i!\n", db_reconnect, db_error);
                }
            } /* while (anOID) */
            gettimeofday(&now, NULL);
            end_time = now.tv_sec * 1000 + now.tv_usec / 1000; /* convert to milliseconds */
            debug(HIGH, "%u getnexts in %.0f ms (%.0f/s)\n", getnexts, end_time - begin_time, (1000 / (end_time - begin_time)) * getnexts);
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
} /* Not reached */
