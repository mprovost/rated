/*
 * This file provides the glue between the database libraries and the rest of the code
 */

/* libtool */
#include <ltdl.h>

#include "rated.h"
#include "rateddbi.h"

#include <sys/param.h>

/*
 * Load a specified database driver and bind its functions
 */
int db_init(config_t *set) {
	lt_dlhandle dbhandle;

	/* initialize libtool */
	if (lt_dlinit() > 0)
		return FALSE;

	dbhandle = lt_dlopen(set->dbdriver);
	if (dbhandle == NULL) {
		debug(LOW, "Couldn't load database driver %s: %s\n", set->dbdriver,(char *) lt_dlerror());
		return FALSE;
	} else {
		debug(LOW, "Loaded database driver %s\n", set->dbdriver);
	}

	/* 
	 * load the dbi functions from the library and bind them to local functions
	 * the function defs are in rtgdbi.h
	 */

	if ((db_test = lt_dlsym(dbhandle, "__db_test")) == NULL) {
		debug(LOW, "Couldn't load db_test: %s\n",(char *) lt_dlerror());
		return FALSE;
	}

	if ((db_status = lt_dlsym(dbhandle, "__db_status")) == NULL) {
		debug(LOW, "Couldn't load db_status: %s\n",(char *) lt_dlerror());
		return FALSE;
	}

	if ((db_connect = lt_dlsym(dbhandle, "__db_connect")) == NULL) {
		debug(LOW, "Couldn't load db_connect: %s\n",(char *) lt_dlerror());
		return FALSE;
	}

	if ((db_disconnect = lt_dlsym(dbhandle, "__db_disconnect")) == NULL) {
		debug(LOW, "Couldn't load db_disconnect: %s\n",(char *) lt_dlerror());
		return FALSE;
	}

	/*
        if ((db_commit= lt_dlsym(dbhandle, "__db_commit")) == NULL) {
		debug(LOW, "Couldn't load db_commit: %s\n",(char *) lt_dlerror());
		return FALSE;
	}
	*/

	if ((db_insert = lt_dlsym(dbhandle, "__db_insert")) == NULL) {
		debug(LOW, "Couldn't load db_insert: %s\n",(char *) lt_dlerror());
		return FALSE;
	}

        db_test();
	return TRUE;
}
