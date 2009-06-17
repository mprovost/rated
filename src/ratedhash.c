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
target_t *tail;
unsigned int entries;

/* Read a target file into a target_t singly linked list.
 * hash_target_file() can be called again to replace the target
 * list. This will lose any state for existing targets however.
 */
target_t *hash_target_file(char *file) {
    target_t *ttgt;
    /* dummy target so we can build the list in the correct order, from the tail */
    target_t dummy;

    entries = 0;

    /* Open the target file */
    debug(LOW, "Reading target list [%s].\n", file);
    if ((yyin = fopen(file, "r")) == NULL) {
        fprintf(stderr, "\nCould not open %s for reading.\n", file);
        return (NULL);
    }

    dummy.next = NULL;
    tail = &dummy;

    yyparse();
    fclose(yyin);

    /* print out target OIDs */
    if (set->verbose >= HIGH) {
        ttgt = dummy.next;
        while (ttgt) {
            debug(HIGH, "%s@%s\n", ttgt->objoid, ttgt->host->host);
            ttgt = ttgt->next;
        }
    }

    debug(LOW, "Successfully created [%d] targets, (%d bytes).\n",
        entries, entries * sizeof(target_t));
    return (dummy.next);
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
