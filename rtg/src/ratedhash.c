/****************************************************************************
   Program:     $Id: rtghash.c,v 1.21 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG target hash table routines
****************************************************************************/

#include "common.h"
#include "rtg.h"

extern FILE *yyin;
extern int yyparse(void);
extern int yylex(void);
extern config_t *set;

static int entries = 0;
hash_t hash;


/* Initialize hash table */
void init_hash() {
	int i;

	hash.table = calloc(HASHSIZE, sizeof(target_t *));
	debug(LOW, "Initialize hash table pointers: %d bytes.\n",
		HASHSIZE * sizeof(target_t *));
}


void init_hash_walk() {
	hash.bucket = 0;
	hash.target = hash.table[hash.bucket];
}


target_t *getNext() {
	target_t *next = NULL;

	while (!(hash.target)) {
		(hash.bucket)++;
		if (hash.bucket >= HASHSIZE) 
			return NULL;
		hash.target = hash.table[hash.bucket];	
	}
	next = hash.target;
	hash.target = (hash.target)->next;
	return next;
}


/* Free hash table memory */
void free_hash() {
	target_t *ptr = NULL;
	target_t *f = NULL;
	int i;

	for(i=0;i<HASHSIZE;i++) {
		ptr = hash.table[i];
		while (ptr) {
			f = ptr;
			ptr = ptr->next;
			if (f->host != NULL) {
				free(f->host);
			}
			free(f);	
		}	
	}
}


/* Hash the target */
unsigned long make_key(const void * entry) {
	const target_t *t = (const target_t *) entry;
	const char *h = (const char *) t->host->host;
	const char *o = (const char *) t->objoid;
	unsigned long hashval;

    if (!h) return 0;

	for (hashval = 0; *o != '\0'; o++)
		hashval = (((unsigned int) *o) | 0x20) + 31 * hashval;
	for (;*h != '\0'; h++)
		hashval = (((unsigned int) *h) | 0x20) + 31 * hashval;
    
    return (hashval % HASHSIZE);
}


/* Set state of all targets to state - used on init, update targets, etc */ 
void mark_targets(int state) {
	target_t *p = NULL;
	int i = 0;

	for (i=0;i<HASHSIZE;i++) {
		p = hash.table[i];
		while (p) {
			p->init = state;
			p = p->next;
		}
	}
}


/* Delete targets marked with state = state */
unsigned int delete_targets(int state) {
	target_t *p = NULL;
	target_t *d = NULL;
	int i = 0;
	unsigned int deleted = 0;

	for (i=0;i<HASHSIZE;i++) {
		p = hash.table[i];
		while (p) {
			d = p;
			p = p->next;
			if (d->init == state) {
				del_hash_entry(d);
				deleted++;
			}
		}
	}
	return deleted;
}


/* Dump the entire target hash [sequential num/bucket number] */
void walk_target_hash() {
	target_t *p = NULL;
	int targets = 0;
	int i = 0;
	
    printf("Dumping Target List:\n");
	for (i=0;i<HASHSIZE;i++) {
		p = hash.table[i];
		while (p) {
			printf("[%d/%d]: %s %s %d %s %s %d\n", targets, i,
				p->host->host, p->objoid, p->bits,
				p->host->community, p->table, p->iid);
			targets++;
			p = p->next;
		}
	}
    printf("Total of %u targets [%u bytes of memory].\n",
	   targets, targets * sizeof(target_t));
}


/* return ptr to target if the entry exists in the named list */
void *in_hash(target_t *entry, target_t *list) {
	while (list) {
		if (compare_targets(entry, list)) {
			debug(HIGH, "Found existing %s %s %s %d\n", list->host,
				list->objoid, list->table, list->iid);
			return list;
		}
		list = list->next;
	}
	return NULL;
}


/* TRUE if target 1 == target 2 */
int compare_targets(target_t *t1, target_t *t2) {
	if (!strcmp(t1->host->host, t2->host->host) &&
		!strcmp(t1->objoid, t2->objoid) &&
		!strcmp(t1->table, t2->table) &&
		!strcmp(t1->host->community, t2->host->community) &&
		(t1->iid == t2->iid)) 
			return TRUE;
	return FALSE;
}


/* Remove an item from the list.  Returns TRUE if successful. */
int del_hash_entry(target_t *new) {
    target_t *p = NULL;
    target_t *prev = NULL;
	unsigned int key;

	key = make_key(new);
	if(in_hash(new, hash.table[key]) == NULL)
		return FALSE;
	
	p = hash.table[key];
	/* assert(p != NULL) */
	while (p) {
		if (compare_targets(p, new)) {
			/* if successor, we need to point predecessor to it (if exists) */
			if (p->next) {
				/* there is a predecessor */
				if (prev) 
					prev->next = p->next;
				/* there is no predecessor */
				else 
					hash.table[key] = p->next;
			}
			/* no successor, delete just this target */
			else {
				if (prev) 
					prev->next = NULL;
				else 
					hash.table[key] = NULL;
			}
			free (p);
			return TRUE;
		}
		prev = p;
		p = p->next;
	}	
	return FALSE;
}


/* Add an entry to hash if it is unique, otherwise free() it */
int add_hash_entry(target_t *new) {
    target_t *p = NULL;
	unsigned int key;

	key = make_key(new);
	p = in_hash(new, hash.table[key]);
	if (p) {
		p->init = LIVE;
		free(new);
		return FALSE;
	} 

	if (!hash.table[key]) {
		hash.table[key] = new;
	} else {
		p = hash.table[key];
		while (p->next) p = p->next;
		p->next = new;
	}
	entries++;
	return TRUE;
}


/* Read a target file into a target_t hash table. 
 * Our hash algorithm roughly randomizes targets, easing SNMP load
 * on end devices during polling.
 * hash_target_file() can be called again to update the target
 * hash.  If hash_target_file() finds new target entries in the file, it
 * adds them to the hash via add_hash_entry().
 * If hash_target_file() finds entries in hash but not in file,
 * it removes said entries from hash via delete_targets().
 * The return values of the addition/deletion functions are then
 * added to/subtracted from the static integer 'entries' which keeps a running
 * count of the total number of targets being polled since the start of rtgpoll.
 */
int hash_target_file(char *file) {
    unsigned int removed = 0;

    /* Open the target file */
	if ((yyin = fopen(file, "r")) == NULL) {
		fprintf(stderr, "\nCould not open %s for reading.\n", file);
		return (-1);
	} 
	debug(LOW, "Reading RTG target list [%s].\n", file);

	mark_targets(STALE);
	yyparse();
	fclose(yyin);
	removed = delete_targets(STALE);
	entries -= removed;
	debug(LOW, "Successfully hashed [%d] targets, (%d bytes).\n",
		entries, entries * sizeof(target_t));
	if (removed > 0)
		debug(LOW, "Removed [%d] stale targets from hash.\n", removed);
	return (entries);
}
