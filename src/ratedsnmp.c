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

short snmp_getnext(void *sessp, oid *anOID, size_t *anOID_len, char *oid_string, unsigned long long *result, struct timeval *current_time) {
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    netsnmp_variable_list *vars;
    netsnmp_session *session;
    int getnext_status = 0;
    short bits;
    char result_string[SPRINT_MAX_LEN];

    pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
    snmp_add_null_var(pdu, anOID, *anOID_len);

    /* TODO check return value */
    snprint_objid(oid_string, SPRINT_MAX_LEN, anOID, *anOID_len);
    session = snmp_sess_session(sessp);
    debug(LOW, "Polling (%s@%s)\n", oid_string, session->peername);

    /* this will free the pdu on error so we can't save them for reuse between rounds */
    getnext_status = snmp_sess_synch_response(sessp, pdu, &response);
    vars = response->variables;

    /* Get the current time */
    gettimeofday(current_time, NULL);

    /* convert the opaque session pointer back into a session struct for debug output */
    session = snmp_sess_session(sessp);
    /* convert the result oid to a string for debug */
    snprint_objid(oid_string, SPRINT_MAX_LEN, vars->name, vars->name_length);

    /* Collect response and process stats */
    PT_MUTEX_LOCK(&stats.mutex);
    if (getnext_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        bits = 32; /* default */

        stats.polls++;
        PT_MUTEX_UNLOCK(&stats.mutex);

        if (set->verbose >= DEBUG) {
            /* this results in something like 'Counter64: 11362777584380' */
            /* TODO check return value */
            snprint_value(result_string, SPRINT_MAX_LEN, vars->name, vars->name_length, vars);
            debug(HIGH, "(%s@%s) %s\n", oid_string, session->peername, result_string);
        }
        switch (vars->type) {
            /*
             * Switch over vars->type and modify/assign result accordingly.
             */
            case ASN_COUNTER64:
                bits = 64;
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
                bits = 0;
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
                    bits = -1;
                }
                break;
            default:
                /* no result that we can use, restart the polling loop */
                /* TODO should we remove this target from the list? */
                *result = 0;
                bits = -1;
        } /* switch (vars->type) */
        debug(LOW, "Setting next: (%s@%s)\n", oid_string, session->peername);
        memmove(anOID, vars->name, vars->name_length * sizeof(oid));
        *anOID_len = vars->name_length;
    } else { 
        bits = -1; /* error */

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

        anOID = NULL;
        anOID_len = 0;
    }

    if (response)
        snmp_free_pdu(response);
    return bits;
}

void *poller(void *thread_args)
{
    worker_t *worker = (worker_t *) thread_args;
    crew_t *crew = worker->crew;
    host_t *host = NULL;
    target_t *head;
    target_t *entry = NULL;
    netsnmp_session *sessp;
    short bits;
    unsigned int getnexts;
    getnext_t *getnext_scratch;
    unsigned long long result = 0;
    unsigned long long insert_val = 0;
    /*
    int cur_work = 0;
    int prev_work = 99999999;
    int loop_count = 0;
    */
    double rate = 0;
    struct timeval current_time;
    struct timeval now;
    double begin_time, end_time;
    /* this forces the db to connect on the first poll (that we have something to insert) */
    int db_reconnect = TRUE;
    int db_error = FALSE;
    oid *anOID;
    size_t *anOID_len;
    char oid_string[SPRINT_MAX_LEN];

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

        /* keep the first entry in the target list */
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
            entry->current = entry->getnexts;

            tdebug(DEVELOP, "processing %s@%s\n", entry->objoid, host->host);

            /* set up the variables for the first poll */
            memmove(anOID, entry->anOID, entry->anOID_len * sizeof(oid));
            *anOID_len = head->anOID_len;

            getnexts = 0;

            gettimeofday(&now, NULL);
            begin_time = tv2ms(now);

            /* keep doing getnexts */
            while (anOID) {
                /* reset variables */
                result = insert_val = 0;
                rate = 0;
                db_error = FALSE;

                /* do the actual snmp poll */
                bits = snmp_getnext(sessp, anOID, anOID_len, oid_string, &result, &current_time);

                if (bits < 0) {
                    /* skip to next oid */
                    tdebug(DEBUG, "bits < 0\n");
                    continue;
                }

                getnexts++;

                /*
                 * checks to see if the getnexts are finished
                 */
                /* check to see if the new oid is smaller than the original target */
                if (*anOID_len < entry->anOID_len) {
                    tdebug(DEBUG, "snmpgetnext done <\n");
                    break;
                /* match against the original target to see if we're going into a different part of the oid tree */
                } else if ((memcmp(&entry->anOID, anOID, entry->anOID_len * sizeof(oid)) != 0)) {
                    tdebug(DEBUG, "snmpgetnext done memcmp\n");
                    print_objid(entry->anOID, entry->anOID_len);
                    print_objid(anOID, *anOID_len);
                    break;
                }

                /*
                 * Walk through the list of getnexts, peeking ahead to the next one.
                 * We do it this way instead of moving the pointer ahead at the end of each
                 * poll so you can insert a new one without needing a doubly linked list.
                 */
                if (entry->getnexts) {
                    tdebug(DEBUG, "entry->getnexts\n");
                    while(entry->current->next) {
                        /* first check against the next entry, this should be the most common case */
                        if (*anOID_len == entry->current->next->anOID_len
                            && memcmp(anOID, entry->current->next->anOID, *anOID_len * sizeof(oid)) == 0) {
                                tdebug(DEBUG, "next memcmp equal\n");
                                entry->current = entry->current->next;
                                break;
                        /* then check against the current entry, this happens in the first loop */
                        } else if (*anOID_len == entry->current->anOID_len
                            && memcmp(anOID, entry->current->anOID, *anOID_len * sizeof(oid)) == 0) {
                                tdebug(DEBUG, "memcmp equal\n");
                                break;
                        /* else snmp_oid_compare to the next one to see if it's lesser or greater */
                        } else {
                            /* if greater, then create and insert a new getnext, this is the first poll for a new oid */
                            if (snmp_oid_compare(anOID, *anOID_len, entry->current->next->anOID, entry->current->next->anOID_len) > 0) {
                                tdebug(DEBUG, "insert next\n");
                                getnext_scratch = malloc(sizeof(getnext_t));
                                getnext_scratch->next = entry->current->next;
                                memmove(getnext_scratch->anOID, anOID, *anOID_len * sizeof(oid));
                                getnext_scratch->anOID_len = *anOID_len;
                                entry->current->next = getnext_scratch;
                                entry->current = getnext_scratch;
                                break;
                            /* if lesser, then that must be an abandoned oid, so delete it and start the comparison over again */
                            /* we already checked for equality above so we can ignore that case */
                            } else {
                                tdebug(DEBUG, "delete next\n");
                                print_objid(anOID, *anOID_len);
                                print_objid(entry->current->next->anOID, entry->current->next->anOID_len);
                                getnext_scratch = entry->current->next;
                                entry->current->next = entry->current->next->next;
                                free(getnext_scratch);
                                continue;
                            }
                        }
                    }
                    /* end of list, append a new one */
                    /* don't append last item twice on second poll */
                    if (entry->current->next == NULL && memcmp(anOID, entry->current->anOID, *anOID_len * sizeof(oid)) != 0) {
                        tdebug(DEBUG, "appending getnext\n");
                        entry->current->next = malloc(sizeof(getnext_t));
                        entry->current->next->next = NULL;
                        memmove(entry->current->next->anOID, anOID, *anOID_len * sizeof(oid));
                        entry->current->next->anOID_len = *anOID_len;
                        entry->current = entry->current->next;
                    }
                /* first target of first poll */
                } else {
                    tdebug(DEBUG, "entry->getnexts == NULL\n");
                    entry->getnexts = malloc(sizeof(getnext_t));
                    entry->getnexts->next = NULL;
                    memmove(entry->getnexts->anOID, anOID, *anOID_len * sizeof(oid));
                    entry->getnexts->anOID_len = *anOID_len;
                    entry->current = entry->getnexts;
                }

                tdebug(DEBUG, "result = %llu, last_value = %llu, bits = %hi\n", result, entry->current->last_value, bits);

                /* zero delta */
                /* TODO zero result on first poll */
                if (result == entry->current->last_value) {
                    tdebug(DEBUG, "zero delta: %llu = %llu\n", result, entry->current->last_value);
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
                } else if (result < entry->current->last_value) {
                    PT_MUTEX_LOCK(&stats.mutex);
                    stats.wraps++;
                    PT_MUTEX_UNLOCK(&stats.mutex);

                    if (bits == 32) insert_val = (THIRTYTWO - entry->current->last_value) + result;
                    else if (bits == 64) insert_val = (SIXTYFOUR - entry->current->last_value) + result;

                    rate = insert_val / timediff(current_time, entry->current->last_time);

                    tdebug(LOW, "*** Counter Wrap (%s@%s) [poll: %llu][last: %llu][insert: %llu]\n",
                        oid_string, host->session.peername, result, entry->current->last_value, insert_val);
                /* Not a counter wrap and this is not the first poll 
                 * the last_time should be 0 on the first poll */
                } else if ((entry->current->last_value >= 0) && (entry->current->last_time.tv_sec > 0)) {
                    insert_val = result - entry->current->last_value;
                    rate = insert_val / timediff(current_time, entry->current->last_time);
                /* entry->current->last_value < 0, so this must be the first poll */
                } else {
                    tdebug(HIGH, "First Poll, Normalizing\n");
                    goto cleanup;
                }

                if (rate) tdebug(DEBUG, "(%lld - %lld = %llu) / (%lums - %lums = %.15fs) = %.15f\n", 
                    result, entry->current->last_value, insert_val, tv2ms(current_time), tv2ms(entry->current->last_time), timediff(current_time, entry->current->last_time), rate);

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

                        /* check if we have a cached value for iid */
                        if (entry->current->iid == 0) {
                            /* get the oid->iid mapping from the db */
                            if (!db_lookup_oid(oid_string, &entry->current->iid)) {
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
                                goto cleanup;
                            }
                        }

                        tdebug(DEVELOP, "db_insert sent: %s %u %llu %.15f\n", entry->table, entry->current->iid, insert_val, rate);
                        /* insert into the database */
                        if (db_insert(entry->table, entry->current->iid, insert_val, rate)) {
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
                        } /* db_insert */
                    } /* zero */
                } /* !dboff */

        /*
                tdebug(HIGH, "doing commit\n");
                db_status = db_commit();
        */

cleanup:
                /* update the time if we inserted ok */
                if (!db_error) {
                    entry->current->last_time = current_time;	
                    /* Only if we received a positive result back do we update the entry->current->last_value */
                    if (bits >= 0) {
                        entry->current->last_value = result;
                    } else {
                        tdebug(DEBUG, "no success:%i!\n", bits);
                    }
                } else {
                    tdebug(DEBUG, "db_reconnect = %i db_error = %i!\n", db_reconnect, db_error);
                }
            } /* while (anOID) */
            gettimeofday(&now, NULL);
            end_time = tv2ms(now);
            tdebug(HIGH, "%u getnexts in %.0f ms (%.0f/s)\n", getnexts, end_time - begin_time, (1000 / (end_time - begin_time)) * getnexts);
            /* loop_count++; */

            if (set->verbose >= HIGH) {
                entry->current = entry->getnexts;
                while (entry->current) {
                    print_objid(entry->current->anOID, entry->current->anOID_len);
                    entry->current = entry->current->next;
                }
            }

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
