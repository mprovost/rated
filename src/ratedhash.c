/****************************************************************************
   Program:     $Id: rtghash.c,v 1.21 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG target hash table routines
****************************************************************************/

#include "common.h"
#include "rated.h"

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
            debug(HIGH, "%s\n", host->host);
            ttgt = host->targets;
            while (ttgt) {
                debug(HIGH, "\t%s\n", ttgt->objoid);
                ttgt = ttgt->next;
            }
            host = host->next;
        }
    }

    debug(LOW, "Successfully created [%i] hosts, [%i] targets, (%i bytes).\n",
        hosts, targets, targets * sizeof(target_t));
    return (host_dummy.next);
}

int free_target_list(target_t *head) {
    unsigned int count;
    target_t *tmp;
    target_t *tmp_next;

    count = 0;

    if (head) {
        tmp = head;
        do {
            count++;
            tmp_next = tmp->next;
            free(tmp);
            tmp = tmp_next;
        } while (tmp);
    }
    return count;
}
