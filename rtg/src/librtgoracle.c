/*
 * RTG Oracle OCI database driver
 */

#include "rtg.h"
#include "rtgplot.h"
#include <oci.h>
#include <sys/param.h>

// thread-specific global variable
pthread_key_t key;

// only call the thread-specific variable setup once 
pthread_once_t once = PTHREAD_ONCE_INIT;

// holds the config info for all the threads
config_t *set;


//
static ARRAY_SIZE= 10;

// Oracle handles
static OCIError   *errhp;
static OCIEnv     *envhp;
static OCICPool   *poolhp;

// stores the Oracle connection pool name and length
static OraText *poolName;
static sb4 poolNameLen;

/* Max,Min, and increment connections */
static ub4 conMin = 1;
static ub4 conIncr = 1;

static OCINumber    global_gid[256];

#define NUMBER_FORMAT_INT (OraText*)"FM222222222222222222222"
#define NUMBER_FORMAT_INT_LEN (sizeof(NUMBER_FORMAT_INT) - 1)


// log any Oracle errors
void checkerr(errhp, status)
OCIError *errhp;
sword status;
{
    text errbuf[512];
    sb4 errcode = 0;
 
    switch (status)
    {
    case OCI_SUCCESS:
      break;
    case OCI_SUCCESS_WITH_INFO:
      debug(LOW, "Error - OCI_SUCCESS_WITH_INFO\n");
      break;
    case OCI_NEED_DATA:
      debug(LOW, "Error - OCI_NEED_DATA\n");
      break;
    case OCI_NO_DATA:
      debug(LOW, "Error - OCI_NODATA\n");
      break;
    case OCI_ERROR:
      (void) OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode,
                          errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
      debug(LOW, "Error - %.*s\n", 512, errbuf);
      break;
    case OCI_INVALID_HANDLE:
      debug(LOW, "Error - OCI_INVALID_HANDLE\n");
      break;
    case OCI_STILL_EXECUTING:
      debug(LOW, "Error - OCI_STILL_EXECUTE\n");
      break;
    case OCI_CONTINUE:
      debug(LOW, "Error - OCI_CONTINUE\n");
      break;
    default:
      break;
    }
}

/* variable cleanup function */
void killkey(void *target) {
   free(target);
   
   // destroy the connection pool
   checkerr(errhp, (sword)OCIConnectionPoolDestroy(poolhp, errhp, OCI_DEFAULT));
   checkerr(errhp, OCIHandleFree((dvoid *)poolhp, OCI_HTYPE_CPOOL));
   checkerr(errhp, OCIHandleFree((dvoid *)errhp, OCI_HTYPE_ERROR));
}

/* called when library loads */
void __attribute__ ((constructor)) dl_init(void) {
}

/* this gets called once */
void my_makekey() {
   /* this shouldn't fail, and we're too early on to report errors */
   pthread_key_create(&key, killkey);

   // create the connection pool
   OCIEnvCreate (&envhp, OCI_THREADED, (dvoid *)0, NULL, NULL, NULL, 0, (dvoid *)0);

   (void)OCIHandleAlloc((dvoid *)envhp, (dvoid**)&errhp, OCI_HTYPE_ERROR, (size_t)0, (dvoid **) 0);

   (void)OCIHandleAlloc((dvoid *) envhp, (dvoid **) &poolhp, OCI_HTYPE_CPOOL, (size_t) 0, (dvoid **) 0);

   checkerr(errhp, OCIConnectionPoolCreate(envhp,
      errhp, poolhp, &poolName, &poolNameLen,
      set->dbdb, (sb4)strlen((const signed char *)set->dbdb),
      conMin, set->threads, conIncr,
      set->dbuser, (sb4)strlen((const signed char *)set->dbuser),
      set->dbpass, (sb4)strlen((const signed char *)set->dbpass),
      OCI_DEFAULT)
   );
   OCIThreadProcessInit();
   checkerr(errhp, OCIThreadInit(envhp, errhp));
}

int __db_test() {
   return TRUE;
}

#ifdef STRTOLL
long long __db_intSpeed(char *query) {
#else
long __db_intSpeed(char *query) {
#endif
    OCISvcCtx *svchp = pthread_getspecific(key);
    OCIStmt *stmthp = (OCIStmt *)0;
    sword lstat = OCI_SUCCESS;
    sword     status;
    boolean   has_more_data = TRUE;
    OCIDefine *defn1p = NULL, *defn2p = NULL;
    double         double_gid;
    int row;
    sword   speed[5];
    int rows_fetched;
    
    debug(LOW,"Time to do the DB query of [%s]\n",query);
    
    OCIHandleAlloc(envhp, (dvoid **)&stmthp, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);

    checkerr(errhp, lstat = OCIStmtPrepare (stmthp, errhp, (CONST OraText *)query, (ub4)strlen((const signed char *)query), OCI_NTV_SYNTAX, OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! Prepare worked!\n");
    } else {
        return FALSE;
    }

    checkerr(errhp, lstat = OCIDefineByPos(stmthp, &defn1p, errhp, (ub4)1,
            (dvoid *)speed,
            (sword)sizeof(sword), SQLT_INT,
            (dvoid *)0, (ub2 *)0, (ub2 *)0,
            (ub4)OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! 1st Define worked!\n");
    } else {
        return FALSE;
    }
    
    checkerr(errhp, lstat = OCIStmtExecute (svchp, stmthp, errhp, (ub4)1, (ub4)0, (OCISnapshot *)0, (OCISnapshot *)0, OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! Execute worked!\n");
    } else {
        return FALSE;
    }


    /* process data */
    checkerr(errhp, OCIAttrGet((dvoid *)stmthp, (ub4)OCI_HTYPE_STMT,
        (dvoid *)&rows_fetched, (ub4 *)0,
        (ub4)OCI_ATTR_ROW_COUNT, errhp));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! AttrGet worked!");
    } else {
        return FALSE;
    } 

    return speed[0];
}

/* Do the query in the DB for object points */
int __db_populate(char *query,data_obj_t *DO) {
    OCISvcCtx *svchp = pthread_getspecific(key);
    OCIStmt *stmthp = (OCIStmt *)0;
    sword lstat = OCI_SUCCESS;
    int       i,
    nrows = 0,
    rows_fetched = 0,
    rows_processed = 0,
    rows_to_process = 0;
    sword     status;
    boolean   has_more_data = TRUE;
    OCIDefine *defn1p = NULL, *defn2p = NULL;
    double         double_gid;
    int row;
    sword dtime[ARRAY_SIZE];
    /* sword counter[ARRAY_SIZE]; */
    OCINumber counter[ARRAY_SIZE];
    int stat = FALSE;
    data_t *new = NULL;
    data_t *last = NULL;
    data_t **data = &(DO->data);



   
    debug(LOW,"Time to do the DB query of [%s]\n",query);
    
    OCIHandleAlloc(envhp, (dvoid **)&stmthp, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);

    checkerr(errhp, lstat = OCIStmtPrepare (stmthp, errhp, (CONST OraText *)query, (ub4)strlen((const signed char *)query), OCI_NTV_SYNTAX, OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! Prepare worked!\n");
    } else {
        return FALSE;
    }

    checkerr(errhp, lstat = OCIDefineByPos(stmthp, &defn1p, errhp, (ub4)2,
            (dvoid *)dtime,
            (sword)sizeof(sword), SQLT_INT,
            (dvoid *)0, (ub2 *)0, (ub2 *)0,
            (ub4)OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! 1st Define worked!\n");
    } else {
        return FALSE;
    }
    checkerr(errhp, lstat = OCIDefineByPos(stmthp, &defn1p, errhp, (ub4)1,
            (dvoid *) &counter,
            (sb4) sizeof(OCINumber), SQLT_VNU,
            (dvoid *)0, (ub2 *)0, (ub2 *)0,
            (ub4)OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! 2nd Define worked!\n");
    } else {
        return FALSE;
    }

    checkerr(errhp, lstat = OCIStmtExecute (svchp, stmthp, errhp, (ub4)1, (ub4)0, (OCISnapshot *)0, (OCISnapshot *)0, OCI_DEFAULT));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! Execute worked!\n");
    } else {
        return FALSE;
    }


    /* process data */
    checkerr(errhp, OCIAttrGet((dvoid *)stmthp, (ub4)OCI_HTYPE_STMT,
        (dvoid *)&rows_fetched, (ub4 *)0,
        (ub4)OCI_ATTR_ROW_COUNT, errhp));
    if (lstat == OCI_SUCCESS) {
        debug(LOW,"yay! AttrGet worked!\n");
    } else {
        return FALSE;
    } 
    rows_to_process = rows_fetched - rows_processed;
    debug(LOW,"rows_fetched = %d - rows_processed = %d\n",rows_fetched,rows_processed);
    debug(LOW,"foo: %ld\n", dtime[0]); 
    if ((new = (data_t *) malloc(sizeof(data_t))) == NULL) {
        debug(LOW,"  Fatal malloc error in populate.\n"); 
        exit(1);
    }

    /* new->counter = (sword) counter[0]; */
debug(LOW,"convert to int!\n");
    checkerr(errhp, lstat = OCINumberToInt(errhp,&counter[0],sizeof(new->counter),OCI_NUMBER_UNSIGNED,&new->counter));
    new->timestamp = (sword) dtime[0];
    new->next = NULL;
    debug(LOW,"counter: %ld ts: %ld\n",new->counter,new->timestamp);
    (DO->datapoints)++;
    if (new->counter > DO->counter_max)
        DO->counter_max = new->counter;
    DO->dataBegin = new->timestamp;
    *data = new; 
    last = new;

    stat = TRUE;

    rows_processed = rows_to_process;
    while (has_more_data) {
        debug(LOW,"has more data....\n");
/*        if ((new = (data_t *) malloc(sizeof(data_t))) == NULL) 
            debug(LOW, "  Fatal malloc error in populate.\n");  */

        status = OCIStmtFetch(stmthp, errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT);

        if (status != OCI_SUCCESS)
            has_more_data = FALSE;

        /* process data */
        checkerr(errhp, OCIAttrGet((dvoid *)stmthp, (ub4)OCI_HTYPE_STMT,
            (dvoid *)&rows_fetched, (ub4 *)0,
            (ub4)OCI_ATTR_ROW_COUNT, errhp));
        rows_to_process = rows_fetched - rows_processed;
        debug(LOW,"rows_fetched = %d - rows_processed = %d\n",rows_fetched,rows_processed);

        for (row = 0; row < rows_to_process; row++, rows_processed++) {
            if ((new = (data_t *) malloc(sizeof(data_t))) == NULL) 
                debug(LOW, "  Fatal malloc error in populate.\n"); 
/*            checkerr(errhp, OCINumberToReal(errhp, &(global_gid[row]),
                (uword)sizeof(double),
                (dvoid *)&double_gid)); */

debug(LOW,"convert to int!\n");
            checkerr(errhp, lstat = OCINumberToInt(errhp,&counter[row],sizeof(new->counter),OCI_NUMBER_UNSIGNED,&new->counter));
            new->timestamp = (sword) dtime[row];
/*            new->next = NULL; */
            debug(LOW,"counter: %ld ts: %ld\n",new->counter,new->timestamp);
            (DO->datapoints)++;
            if (new->counter > DO->counter_max)
                DO->counter_max = new->counter;
            last->next = new;
            last = new;
/*            printf("\n%ld %ld\n", dtime[row], counter[row]); */
        }
    }
    
    new->next = NULL;
/*    *data = new; */
    DO->dataEnd = new->timestamp;

    if (status != OCI_SUCCESS_WITH_INFO && status != OCI_NO_DATA)
        checkerr(errhp, status);
   
    checkerr(errhp, lstat = OCIHandleFree((dvoid *)stmthp, OCI_HTYPE_STMT));
    return stat;
}

/*
 * check the status of the connection
 * we don't try and reconnect because this is sometimes used to confirm a disconnect
 */
int __db_status() {
   sword lstat = OCI_SUCCESS;
   char buf[100];

   OCISvcCtx *svchp = pthread_getspecific(key);
   checkerr(errhp, lstat = OCIServerVersion(svchp, errhp, buf, 100, OCI_HTYPE_SVCCTX));
   if (lstat == OCI_SUCCESS) {
      return TRUE;
   } else {
      return FALSE;
   }
}

int __db_connect(config_t *config) {
   // initialize the key and connection pool once
   set = config;
   pthread_once(&once, my_makekey);
   // connect to Oracle
   OCISvcCtx *svchp = (OCISvcCtx *)0;
   checkerr(errhp, OCILogon2(envhp, errhp, &svchp,
      (CONST OraText *)config->dbuser, (ub4)strlen((const signed char *)config->dbuser),
      (CONST OraText *)config->dbpass, (ub4)strlen((const signed char *)config->dbpass),
      (CONST OraText *)poolName, (ub4)poolNameLen, OCI_CPOOL)
   );

   // store the Oracle session in thread-specific storage
   pthread_setspecific(key, svchp);

   return __db_status();
}

int __db_disconnect() {
   OCISvcCtx *svchp = pthread_getspecific(key);
   checkerr(errhp, OCILogoff((dvoid *) svchp, errhp));
   return TRUE;
}

int __db_commit() {
   sword lstat;
   struct timespec ts1;
   struct timespec ts2;
   unsigned int ms_took;

   clock_gettime(CLOCK_REALTIME,&ts1);
   OCISvcCtx *svchp = pthread_getspecific(key);
   
   checkerr(errhp, lstat = OCITransCommit(svchp, errhp, (ub4)0));
  
   clock_gettime(CLOCK_REALTIME,&ts2);
   ms_took = (unsigned int)((ts2.tv_sec * 1000000000 + ts2.tv_nsec) - (ts1.tv_sec * 1000000000 + ts1.tv_nsec)) / 1000000;
/*    if( ms_took > 10 ) { */
        debug(HIGH,"Commit took %d milliseconds at [%d][%d][%d][%d]\n",ms_took,ts1.tv_sec,ts1.tv_nsec,ts2.tv_sec,ts2.tv_nsec);
/*   } */
   return TRUE;
}

int __db_insert(char *table, int iid, unsigned long long insert_val, double insert_rate) {
   char *query;
   OCISvcCtx *svchp = pthread_getspecific(key);
   OCIStmt *stmthp = (OCIStmt *)0;
   OCIBind *bnd1p = (OCIBind *) 0;
   OCIBind *bnd2p = (OCIBind *) 0;
   OCIBind *bnd3p = (OCIBind *) 0;
   OCIBind *bnd4p = (OCIBind *) 0;
   sword lstat;
   time_t cur_time = time(NULL);
   OCINumber oci_val;
   char *ins_buffer;
   int foo;
   struct timespec ts1;
   struct timespec ts2;
   unsigned int ms_took;

   clock_gettime(CLOCK_REALTIME,&ts1);


   /* asprintf(&query, "INSERT INTO %s (id,dtime,counter,rate) VALUES (%i,%d,%llu,%lf)", table, iid, time(NULL), insert_val, insert_rate); */
   /* asprintf(&query, "INSERT INTO %s (id,dtime,counter,rate) VALUES (:IID,:TIME,:INSERTVAL,:INSERTRATE)", table); */
   asprintf(&query, "INSERT INTO %s (id,dtime,counter,rate) VALUES (:iid,:time,:insertval,:insertrate)", table);

   debug(DEVELOP, "Query = \"%s\"\n", query);
/*
    checkerr(errhp, lstat = OCIDefineByPos(stmthp, &defn1p, errhp, (ub4)1,
            (dvoid *)counter,
            (sword)sizeof(sword), SQLT_INT,
            (dvoid *)0, (ub2 *)0, (ub2 *)0,
            (ub4)OCI_DEFAULT));
*/
   OCIHandleAlloc(envhp, (dvoid **)&stmthp, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);

   checkerr(errhp, lstat = OCIStmtPrepare (stmthp, errhp, (CONST OraText *)query, (ub4)strlen((const signed char *)query), OCI_NTV_SYNTAX, OCI_DEFAULT));
   
   checkerr(errhp, lstat = OCINumberFromInt(errhp,(dvoid *) &insert_val,sizeof(insert_val),OCI_NUMBER_UNSIGNED,&oci_val)); 
   checkerr(errhp, lstat = OCINumberToInt(errhp,&oci_val,sizeof(foo), OCI_NUMBER_UNSIGNED,&foo));

   checkerr(errhp, lstat = OCIBindByName(stmthp, &bnd1p, errhp, 
        (text *) ":IID", strlen(":IID"), (ub1 *) &iid, (sword) sizeof(iid),
        SQLT_INT, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));
   checkerr(errhp, lstat = OCIBindByName(stmthp, &bnd2p, errhp, 
        (text *) ":TIME", strlen(":TIME"), (ub1 *) &cur_time, (sword) sizeof(cur_time),
        SQLT_INT, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));
/*   checkerr(errhp, lstat = OCIBindByName(stmthp, &bnd3p, errhp, 
        (text *) ":INSERTVAL", strlen(":INSERTVAL"), (ub1 *) &insert_val, (sword) sizeof(insert_val),
        SQLT_NUM, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT)); */
   checkerr(errhp, lstat = OCIBindByName(stmthp, &bnd3p, errhp, 
        (text *) ":INSERTVAL", strlen(":INSERTVAL"), (ub1 *) &oci_val, (ub4) sizeof(unsigned char[22]),
        SQLT_VNU, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT)); 
   checkerr(errhp, lstat = OCIBindByName(stmthp, &bnd4p, errhp, 
        (text *) ":INSERTRATE", strlen(":INSERTRATE"), (ub1 *) &insert_rate, (sword) sizeof(insert_rate),
        SQLT_INT, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

   checkerr(errhp, lstat = OCIStmtExecute (svchp, stmthp, errhp, (ub4)1, (ub4)0, (OCISnapshot *)0, (OCISnapshot *)0, OCI_DEFAULT));

/*   checkerr(errhp, lstat = OCITransCommit(svchp, errhp, (ub4)0));  */

   checkerr(errhp, lstat = OCIHandleFree((dvoid *)stmthp, OCI_HTYPE_STMT));

   clock_gettime(CLOCK_REALTIME,&ts2);


   ms_took = (unsigned int)((ts2.tv_sec * 1000000000 + ts2.tv_nsec) - (ts1.tv_sec * 1000000000 + ts1.tv_nsec)) / 1000000;
   if( ms_took > 10 ) {
        debug(HIGH,"Query took %d milliseconds \"%s\"\n",ms_took,query);
   }

   free(query);

   if (lstat == OCI_SUCCESS) {
      return TRUE;
   } else {
      return FALSE;
   }
}
