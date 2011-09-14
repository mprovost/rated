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
extern sig_atomic_t quitting_now;

void cancel_lock(void *arg)
{
    /* to be called from the cleanup handler */
    PT_MUTEX_UNLOCK(arg);
}

void cleanup_db(void *arg)
{
    db_disconnect();
}

/* check the host uptime via snmp
 * return timeticks or 0 on error */
unsigned long snmp_sysuptime(worker_t *worker, netsnmp_session *sessp) {
    /* DISMAN-EVENT-MIB::sysUpTimeInstance = .1.3.6.1.2.1.1.3.0 */
    oid sysuptimeOID[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
    char *oid_string = ".1.3.6.1.2.1.1.3.0";
    size_t sysuptimeOID_len = 9;
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    netsnmp_variable_list *vars;
    netsnmp_session *session;
    char result_string[SPRINT_MAX_LEN];
    char *errstr;
    int liberr, syserr;
    int sysuptime_status;
    unsigned long timeticks = 0;

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, sysuptimeOID, sysuptimeOID_len);
    /* convert the opaque session pointer back into a session struct for debug output */
    session = snmp_sess_session(sessp);

    /* do the snmp query */
    sysuptime_status = snmp_sess_synch_response(sessp, pdu, &response);

    if (sysuptime_status == STAT_SUCCESS && response && response->errstat == SNMP_ERR_NOERROR){
        vars = response->variables;
        if (vars->type == ASN_TIMETICKS) {
            if (set->verbose >= DEBUG) {
                snprint_value(result_string, SPRINT_MAX_LEN, vars->name, vars->name_length, vars);
                tdebug_all("sysuptime (%s@%s) %s\n", oid_string, session->peername, result_string);
            }
            timeticks = (unsigned long) *(vars->val.integer);
        } else {
            if (set->verbose >= LOW) {
                snprint_value(result_string, SPRINT_MAX_LEN, vars->name, vars->name_length, vars);
                tdebug_all("sysuptime incorrect data type, expecting Timeticks: (%s@%s) %s\n", oid_string, session->peername, result_string);
            }
        }
    } else {
        PT_MUTEX_LOCK(&stats.mutex);
        switch (sysuptime_status) {
            case STAT_TIMEOUT:
                stats.no_resp++;
                tdebug(LOW, "*** SNMP No response: (%s@%s).\n", oid_string, session->peername);
                break;
            case STAT_SUCCESS:
                stats.errors++;
                if (response) {
                    if (response->variables->type == SNMP_NOSUCHINSTANCE) {
                        tdebug(LOW, "*** SNMP Error: No Such Instance Exists (%s@%s)\n", oid_string, session->peername);
                    } else {
                        snmp_error(session, &liberr, &syserr, &errstr);
                        tdebug(LOW, "*** SNMP Error: (%s@%s) %s\n", oid_string, session->peername, errstr);
                        free(errstr);
                    }
                } else {
                    tdebug(LOW, "*** SNMP NULL response: (%s@%s)\n", oid_string, session->peername);
                }
                break;
            default: /* STAT_ERROR */
                stats.errors++;
                snmp_error(session, &liberr, &syserr, &errstr);
                tdebug(LOW, "*** SNMP Error: (%s@%s) %s\n", oid_string, session->peername, errstr);
                break;
        }
        PT_MUTEX_UNLOCK(&stats.mutex);
    }

    if (response)
        snmp_free_pdu(response);

    return timeticks;
}

short snmp_getnext(worker_t *worker, void *sessp, oid *anOID, size_t *anOID_len, char *oid_string, unsigned long long *result, struct timeval *current_time, template_t *current_template) {
    crew_t *crew = worker->crew;
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    netsnmp_variable_list *vars;
    netsnmp_session *session;
    char *errstr;
    int liberr, syserr;
    int getnext_status = 0;
    short bits = 32; /* default */
    char result_string[SPRINT_MAX_LEN];

    result_string[0] = '\0';
    pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
    snmp_add_null_var(pdu, anOID, *anOID_len);

    /* TODO check return value */
    snprint_objid(oid_string, SPRINT_MAX_LEN, anOID, *anOID_len);
    /* convert the opaque session pointer back into a session struct for debug output */
    session = snmp_sess_session(sessp);
    tdebug(HIGH, "Polling (%s@%s)\n", oid_string, session->peername);

    /* this will free the pdu so we can't save them for reuse between rounds */
    getnext_status = snmp_sess_synch_response(sessp, pdu, &response);

    /* Get the current time */
    gettimeofday(current_time, NULL);

    /* Collect response and process stats */
    /* check for NULL responses */
    if (getnext_status == STAT_SUCCESS && response && response->errstat == SNMP_ERR_NOERROR) {
        vars = response->variables;
        /* convert the result oid to a string for debug */
        snprint_objid(oid_string, SPRINT_MAX_LEN, vars->name, vars->name_length);

        if (set->verbose >= DEBUG) {
            /* this results in something like 'Counter64: 11362777584380' */
            /* TODO check return value */
            snprint_value(result_string, SPRINT_MAX_LEN, vars->name, vars->name_length, vars);
            tdebug_all("(%s@%s) %s\n", oid_string, session->peername, result_string);
        }

        /*
         * checks to see if the getnexts are finished
         */
        /* check to see if the new oid is smaller than the original target */
        if (vars->name_length < current_template->anOID_len ||
        /* match against the original target to see if we're going into a different part of the oid tree */
           (memcmp(&current_template->anOID, vars->name, current_template->anOID_len * sizeof(oid)) != 0)) {
            tdebug(DEBUG, "snmp_getnext done: %s > %s\n", oid_string, current_template->objoid);
            bits = -1;
            /* zero anOID so we break out of the getnext loop */
            memset(anOID, 0, *anOID_len * sizeof(oid));
            *anOID_len = 0;
        /* valid getnext, process */
        } else {
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
                    bits = 0;
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
                        tdebug(LOW, "Negative number found: %s\n", vars->val.string);
                        bits = -1;
                    }
                    break;
                default:
                    /* no result that we can use, skip to the next target */
                    PT_MUTEX_LOCK(&stats.mutex);
                    stats.unusable++;
                    PT_MUTEX_UNLOCK(&stats.mutex);
                    if (set->verbose >= LOW) {
                        if (strlen(result_string) == 0) 
                            snprint_value(result_string, SPRINT_MAX_LEN, vars->name, vars->name_length, vars);
                        tdebug_all("Unusable result: (%s@%s) %s\n", oid_string, session->peername, result_string);
                    }
                    *result = 0;
                    bits = -1;
            } /* switch (vars->type) */
            tdebug(DEBUG, "Setting next: (%s@%s)\n", oid_string, session->peername);
            memmove(anOID, vars->name, vars->name_length * sizeof(oid));
            *anOID_len = vars->name_length;
        }
    } else { 
        bits = -1; /* error */
        /* if we didn't get a result back, zero anOID so we break out of the getnext loop */
        memset(anOID, 0, *anOID_len * sizeof(oid));
        *anOID_len = 0;

        if (session) {
            snmp_error(session, &liberr, &syserr, &errstr);
        } else {
            errstr = "NULL session";
        }

        PT_MUTEX_LOCK(&stats.mutex);
        switch (getnext_status) {
            case STAT_TIMEOUT:
                stats.no_resp++;
                tdebug(LOW, "*** SNMP No response: (%s@%s).\n", oid_string, session->peername);
                break;
            case STAT_SUCCESS:
                stats.errors++;
                if (response) {
                    if (response->variables->type == SNMP_NOSUCHINSTANCE) {
                        tdebug(LOW, "*** SNMP Error: No Such Instance Exists (%s@%s)\n", oid_string, session->peername);
                    } else {
                        tdebug(LOW, "*** SNMP Error: (%s@%s) %s\n", oid_string, session->peername, errstr);
                    }
                } else {
                    tdebug(LOW, "*** SNMP NULL response: (%s@%s)\n", oid_string, session->peername);
                }
                break;
            default: /* STAT_ERROR */
                stats.errors++;
                tdebug(LOW, "*** SNMP Error: (%s@%s) Unsuccessful (%s).\n", oid_string, session->peername, errstr);
                break;
        }
        PT_MUTEX_UNLOCK(&stats.mutex);

        free(errstr);
    }

    if (response)
        snmp_free_pdu(response);

    return bits;
}

/* insert/append a new getnext into a list */
getnext_t *insert_getnext(getnext_t *current_getnext, const oid *anOID, const size_t anOID_len) {
    getnext_t *getnext_scratch;

    /* construct a new one */
    getnext_scratch = calloc(1, sizeof(getnext_t));
    memmove(getnext_scratch->anOID, anOID, anOID_len * sizeof(oid));
    getnext_scratch->anOID_len = anOID_len;

    /* check for empty list */
    if (current_getnext) {
        /* not empty */
        /* this could be NULL if we're appending to the end of the list */
        getnext_scratch->next = current_getnext->next;
        /* insert/append to the list */
        current_getnext->next = getnext_scratch;
        current_getnext = getnext_scratch;
    } else {
        /* empty */
        current_getnext = getnext_scratch;
    }

    return current_getnext;
}

/*
 * Walk through the list of getnexts, peeking ahead to the next one.
 * We do it this way instead of moving the pointer ahead at the end of each
 * poll so you can insert a new one without needing a doubly linked list.
 */
getnext_t *walk_getnexts(worker_t *worker, getnext_t *current_getnext, const oid *anOID, const size_t anOID_len) {
    getnext_t *next_getnext;
    
    if (current_getnext) {
        next_getnext = current_getnext->next;

        /* return early to avoid the append check every time */
        while(next_getnext) {
            /* first check against the next entry, this should be the most common case */
            if (anOID_len == next_getnext->anOID_len
                && memcmp(anOID, next_getnext->anOID, anOID_len * sizeof(oid)) == 0) {
                    tdebug(DEBUG, "next memcmp equal\n");
                    current_getnext = next_getnext;
                    return current_getnext;
            /* then check against the current entry, this happens in the first loop */
            } else if (anOID_len == current_getnext->anOID_len
                && memcmp(anOID, current_getnext->anOID, anOID_len * sizeof(oid)) == 0) {
                    tdebug(DEBUG, "memcmp equal\n");
                    return current_getnext;
            /* else snmp_oid_compare to the next one to see if it's lesser or greater */
            } else {
                /* if greater, then create and insert a new getnext, this is the first poll for a new oid */
                if (snmp_oid_compare(anOID, anOID_len, next_getnext->anOID, next_getnext->anOID_len) > 0) {
                    tdebug(DEBUG, "insert next\n");
                    current_getnext = insert_getnext(current_getnext, anOID, anOID_len);
                    return current_getnext;
                /* if lesser, then that must be an abandoned oid, so delete it and start the comparison over again */
                /* we already checked for equality above so we can ignore that case */
                } else {
                    tdebug(DEBUG, "delete next\n");
                    current_getnext->next = next_getnext->next;
                    free(next_getnext);
                    next_getnext = current_getnext->next;
                    continue;
                }
            }
        }
    } else {
        /* empty list */
        tdebug(DEBUG, "empty getnext list, creating\n");
        /* insert_getnext will create a new list if it's empty */
    }
    /* either it's an empty list or we've gotten to the end and need to append a new item */
    return insert_getnext(current_getnext, anOID, anOID_len);
}

/* insert into the database and return a boolean indicating whether to attempt to reconnect to the db the next time */
int do_insert(worker_t *worker, int db_reconnect, unsigned long long result, getnext_t *getnext, short bits, struct timeval current_time, const char *oid_string, host_t *host) { 
    crew_t *crew = worker->crew;
    unsigned long long insert_val = 0;
    double rate = 0;
    int db_error = FALSE;

    /* zero delta */
    if (result == getnext->last_value) {
        tdebug(DEBUG, "zero delta: %llu = %llu\n", result, getnext->last_value);
        PT_MUTEX_LOCK(&stats.mutex);
        stats.zero++;
        PT_MUTEX_UNLOCK(&stats.mutex);
        if (set->withzeros) {
            /* check for gauges */
            if (bits == 0) {
                insert_val = result;
            } else {
                insert_val = 0;
            }
        } else {
            goto cleanup;
        }
    /* gauges */
    } else if (bits == 0) {
        insert_val = result;
    /* treat all other values as counters */
    /* Counter Wrap Condition */
    /* TODO check sysuptime again if we detect a counter wrap */
    } else if (result < getnext->last_value) {
        PT_MUTEX_LOCK(&stats.mutex);
        /* check for off by ones
         * some devices seem to occasionally go backwards by one
         * it's extremely unlikely that it's a full counter wrap
         */ 
        if (getnext->last_value - result == 1) {
            stats.obo++;
            insert_val = 0;
        } else if (bits == 32) {
            stats.wraps++;
            insert_val = (THIRTYTWO - getnext->last_value) + result;
        } else if (bits == 64) {
            stats.wraps64++;
            insert_val = (SIXTYFOUR - getnext->last_value) + result;
        }
        PT_MUTEX_UNLOCK(&stats.mutex);

        if (insert_val) {
            tdebug(LOW, "*** Counter Wrap (%s@%s) [poll: %llu][last: %llu][insert: %llu]\n",
                oid_string, host->host, result, getnext->last_value, insert_val);
            rate = insert_val / timediff(current_time, getnext->last_time);
        } else {
            tdebug(LOW, "*** Off by one (%s@%s): %llu - %llu == 1\n", oid_string, host->host, getnext->last_value, result);
        }
    /* Not a counter wrap and this is not the first poll 
     * the last_time should be 0 on the first poll */
    } else if ((getnext->last_value >= 0) && (getnext->last_time.tv_sec > 0)) {
        insert_val = result - getnext->last_value;
        /* rate = per second */
        rate = (double)insert_val / timediff(current_time, getnext->last_time) * 1000;
    /* getnext->last_value < 0, so this must be the first poll */
    } else {
        tdebug(HIGH, "First Poll, Normalizing\n");
        goto cleanup;
    }

    if (rate) tdebug(DEBUG, "(%lld - %lld = %llu) / (%lums - %lums = %lums) = %.15f/s\n", 
        result, getnext->last_value, insert_val, tv2ms(current_time), tv2ms(getnext->last_time), timediff(current_time, getnext->last_time), rate);

    /* TODO do we need to check for zero values again? */
    /*
     * insert into the db
     */
    if (set->dbon) {
        if ( (insert_val > 0) || (set->withzeros) ) {
            if (db_reconnect) {
                /* try and (re)connect */
                if (db_connect(set)) {
                    db_reconnect = FALSE;
                } else {
                    /* the db driver will print an error itself */
                    db_error = TRUE;
                    goto cleanup;
                }
            }

            /* Now we have a DB connection */

            /* check if we already have an iid, otherwise look it up first */
            if (getnext->iid == 0) {
                /* look up the iid in the db */
                if (!db_lookup_oid(oid_string, &getnext->iid)) {
                    db_error = TRUE;
                    /* can't insert without an iid so bail out */
                    goto cleanup;
                }
            }

            tdebug(DEVELOP, "db_insert sent: %s %lu %lu %llu %.15f\n", host->host_esc, getnext->iid, tv2ms(current_time), insert_val, rate);
            /* insert into the database */
            if (db_insert(host->host_esc, getnext->iid, current_time, insert_val, rate)) {
                PT_MUTEX_LOCK(&stats.mutex);
                stats.db_inserts++;
                PT_MUTEX_UNLOCK(&stats.mutex);
            } else {
                db_error = TRUE;
            } /* db_insert */
        } /* zero */
    } /* dbon */

cleanup:

    /* update the time if we inserted ok */
    if (!db_error) {
        getnext->last_time = current_time;	
        /* Only if we received a positive result back do we update the getnext->last_value */
        if (bits >= 0) {
            getnext->last_value = result;
        } else {
            tdebug(DEBUG, "no success:%i!\n", bits);
        }
    } else {
        tdebug(DEBUG, "db_reconnect = %i db_error = %i!\n", db_reconnect, db_error);
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

    return db_reconnect;
}


void *poller(void *thread_args)
{
    worker_t *worker = (worker_t *) thread_args;
    crew_t *crew = worker->crew;
    host_t *host = NULL;
    template_t *current_template;
    target_t *current_target;
    getnext_t *current_getnext;
    netsnmp_session *sessp;
    short bits;
    unsigned long long result = 0;
    unsigned long sysuptime = 0;
    /*
    int cur_work = 0;
    int prev_work = 99999999;
    int loop_count = 0;
    */
    struct timeval current_time;
    struct timeval now;
    /* this forces the db to connect on the first poll (that we have something to insert) */
    int db_reconnect = TRUE;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    char oid_string[SPRINT_MAX_LEN];

    /* for thread settings */
    int oldstate, oldtype;

    tdebug(HIGH, "starting.\n");

    pthread_cleanup_push(cleanup_db, NULL);

    /* set up cancel function for exit */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

    /*
     * crew->running is initialised to the number of threads, count
     * down to 0 as we start up this means that from the first poll
     * we can count up again as we get a target and back down when
     * we're finished to tell when all the threads are done each 
     * round
     */
    PT_MUTEX_LOCK(&crew->mutex);
    crew->running--;
    tdebug(HIGH, "running = %i\n", crew->running);
    PT_MUTEX_UNLOCK(&crew->mutex);

    while (1) {
	/* see if we've been cancelled before we start another loop */
	pthread_testcancel();

        if (quitting_now)
            break;

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

        /* TODO do we need this check at all? */
	if (crew->current) {
            host = crew->current;
            crew->current = crew->current->next;
        } /* TODO else */

        /* we have a host, release the lock */
	tdebug(DEVELOP, "unlocking (done grabbing current = %s)\n", host->host);
        crew->running++;
        tdebug(HIGH, "running++ = %i\n", crew->running);
	PT_MUTEX_UNLOCK(&crew->mutex);

        /* take the unlock off the cancel stack */
        pthread_cleanup_pop(FALSE);

        /* open an snmp session once for all targets for this host for this round */
        /* this makes a copy of the struct from host->session */
        sessp = snmp_sess_open(&host->session);

        if (sessp == NULL) {
            /* skip to next host */
            goto cleanup;
        }

        /* first get the uptime for the host so we can check if it has restarted */
        sysuptime = snmp_sysuptime(worker, sessp);

        if (sysuptime == 0) {
            tdebug(LOW, "Couldn't get sysuptime from %s\n", host->host);
            /* skip to next host */
            goto cleanup;
        }

        /* if the host reset */
        /* TODO check if we were within one poll's time of the 32 bit timestamp */
        if (host->sysuptime && sysuptime < host->sysuptime) {
            tdebug(LOW, "system uptime went backwards on %s (%lu < %lu)!\n", host->host, sysuptime, host->sysuptime);

            /* erase the getnexts, they aren't valid anymore if the host reset */
            free_target_list(host->targets);
            host->targets = calloc(1, sizeof(target_t));
        }

        current_template = host->template;
        current_target = host->targets;
        current_getnext = current_target->getnexts;
                
        /* loop through the targets for this host */
        while (current_template) {
            if (quitting_now)
                break;

            tdebug(DEVELOP, "processing %s@%s\n", current_template->objoid, host->host);

            /* set up the variables for the first poll */
            memmove(&anOID, current_template->anOID, current_template->anOID_len * sizeof(oid));
            anOID_len = current_template->anOID_len;

            /* update the time in the previous poll getnext struct so that we don't count the idle time between polls */
            gettimeofday(&now, NULL);
            current_target->poll.last_time = now;

            /* keep doing getnexts */
            while (anOID_len) {
                if (quitting_now)
                    break;

                /* reset variables */
                result = 0;

                /* do the actual snmp poll */
                bits = snmp_getnext(worker, sessp, anOID, &anOID_len, oid_string, &result, &current_time, current_template);

                /* this is a counter so we never zero it */
                /* count the getnext even if it was an error or returned an unusable result */
                current_target->getnext_counter++;

                /* check for errors/unusable results */
                if (bits < 0) {
                    tdebug(DEBUG, "bits < 0\n");
                    /* skip to next oid without doing an insert */
                    /* if there was an snmp error or we've reached the end of this part of the tree,
                    anOID_len will be zeroed and we'll break out of the while and go to the next target */
                    continue;
                }

                current_getnext = walk_getnexts(worker, current_getnext, anOID, anOID_len);

                /* doublecheck */
                assert(memcmp(&current_getnext->anOID, &anOID, anOID_len) == 0);

                /* check if we started with an empty list */
                /* TODO do we just want to create a new list if it's empty at the start of the loop? */
                if (current_target->getnexts == NULL)
                    /* copy the newly created list into place */
                    current_target->getnexts = current_getnext;

                tdebug(DEBUG, "result = %llu, last_value = %llu, bits = %hi\n", result, current_getnext->last_value, bits);

                /*
                 * insert into the database
                 */
                db_reconnect = do_insert(worker, db_reconnect, result, current_getnext, bits, current_time, oid_string, host);

            } /* while (anOID_len) */
            if (quitting_now)
                break;

            /* grab the time that we finished this target for the internal stats insert */
            gettimeofday(&now, NULL);

            /* just update the stats once so we can avoid locking and unlocking on each getnext */
            /* have to do this before inserting the poll data which will update the last_value */
            PT_MUTEX_LOCK(&stats.mutex);
            stats.polls += current_target->getnext_counter - current_target->poll.last_value;
            PT_MUTEX_UNLOCK(&stats.mutex);
            tdebug(DEBUG, "getnext_counter = %llu - %llu = %llu\n", current_target->getnext_counter, current_target->poll.last_value, current_target->getnext_counter - current_target->poll.last_value);

            /* insert the internal poll data (ie how many getnexts for this oid) into the host table */
            tdebug(DEBUG, "Inserting poll data for %s@%s\n", current_template->objoid, host->host);
            db_reconnect = do_insert(worker, db_reconnect, current_target->getnext_counter, &current_target->poll, 64, now, current_template->objoid, host);

            if (set->verbose >= HIGH) {
                current_getnext = current_target->getnexts;
                while (current_getnext) {
                    print_objid(current_getnext->anOID, current_getnext->anOID_len);
                    current_getnext = current_getnext->next;
                }
                current_getnext = current_target->getnexts;
            }

            /* TODO check for and delete orphaned getnexts at the end of the list */

            /* move to next target in template */
            current_template = current_template->next;
            /* if we're at the end of the targets list and we still have templates we need to make a new one */
            if (current_template && current_target->next == NULL) {
                current_target->next = calloc(1, sizeof(target_t));
            }
            current_target = current_target->next;
            current_getnext = current_target->getnexts;

        } /* while (current_template) */

        /* done with targets for this host */

        /* update timestamp */
        host->sysuptime = sysuptime;

cleanup:

        if (sessp != NULL)
            snmp_sess_close(sessp);

        PT_MUTEX_LOCK(&crew->mutex);
        crew->running--;
        tdebug(HIGH, "running-- = %i\n", crew->running);
        if (quitting_now)
            tdebug(HIGH, "Quitting early! Broadcasting thread done condition.\n");
        else
            tdebug(HIGH, "Queue processed. Broadcasting thread done condition.\n");
        /* this will wake up the main thread which will see if any other threads are still running */
        /* TODO change to pthread_cond_signal? */
        PT_COND_BROAD(&crew->done);
        PT_MUTEX_UNLOCK(&crew->mutex);
    } /* while(1) */
    pthread_cleanup_pop(FALSE);
} /* Not reached */
