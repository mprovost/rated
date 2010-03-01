/****************************************************************************
   Program:     $Id: rtghash.c,v 1.21 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG target hash table routines
****************************************************************************/

#include "common.h"
#include "rated.h"
#include "ratedsnmp.h"

extern FILE *yyin;
extern int yyparse(void);
extern int yylex(void);
extern config_t *set;

/* globals for yacc */
host_t *hosts_tail;
unsigned int hosts;
unsigned int targets;

/* Read a target file into a target_t singly linked list.
 * hash_target_file() can be called again to replace the target
 * list. This will lose any state for existing targets however.
 */
host_t *hash_target_file(char *file) {
    host_t *host;
    target_t *ttgt;
    /* dummy target so we can build the list in the correct order, from the tail */
    host_t host_dummy;

    hosts = 0;
    targets = 0;

    /* Open the target file */
    debug(LOW, "Reading target list [%s].\n", file);
    if ((yyin = fopen(file, "r")) == NULL) {
        fprintf(stderr, "\nCould not open %s for reading.\n", file);
        return (NULL);
    }

    host_dummy.next = NULL;
    hosts_tail = &host_dummy;

    yyparse();
    fclose(yyin);

    /* print out target OIDs */
    if (set->verbose >= HIGH) {
        host = host_dummy.next;
        while (host) {
            debug_all("%s\n", host->host);
            print_targets(host->targets);
            host = host->next;
        }
    }

    debug(LOW, "Successfully created [%i] hosts, [%i] targets (%i bytes).\n",
        hosts, targets, targets * sizeof(target_t) + hosts * sizeof(host_t));
    return (host_dummy.next);
}

void print_targets(target_t *targets) {
    while (targets) {
        debug_all("\t%s\n", targets->objoid);
        targets = targets->next;
    }
}

/* walk through the host/target list freeing everything */
int free_host_list(host_t *head) {
    unsigned int count = 0;
    host_t *host;
    host_t *host_next;

    if (head) {
        host = head;
        do {
            count++;
            if (host->targets) {
                free_target_list(host->targets);
            }
            host_next = host->next;
            free(host);
            host = host_next;
        } while (host);
    }
    return count;
}

int free_target_list(target_t *head) {
    unsigned int count = 0;
    target_t *target_next;

    do {
        count++;
        target_next = head->next;
        free(head);
        head = target_next;
    } while (head);

    return count;
}

int free_getnext_list(getnext_t *head) {
    unsigned int count = 0;
    getnext_t *getnext_next;

    do {
        count++;
        getnext_next = head->next;
        free(head);
        head = getnext_next;
    } while (head);

    return count;
}

/* deep copy a target list */
/* doesn't copy the getnexts list */
target_t *copy_target_list(target_t *head) {
    target_t *current = head;
    target_t dummy;
    target_t *tail = &dummy;

    dummy.next = NULL;

    while (current) {
        tail->next = calloc(1, sizeof(target_t));
        tail = tail->next;
        tail->objoid = current->objoid;
        //tail->anOID = current->anOID;
        memcpy(tail->anOID, current->anOID, MAX_OID_LEN);
        tail->anOID_len = current->anOID_len;
        tail->getnexts = NULL;
        tail->current = NULL;
        tail->next = NULL;
        current = current->next;
    }
    return dummy.next;
}
