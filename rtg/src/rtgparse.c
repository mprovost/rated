/****************************************************************************
   Program:     $Id: rtgparse.c,v 1.11 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG string parsing routines
****************************************************************************/

#include "common.h"
#include "rtg.h"
#include "rtgplot.h"
 
extern data_obj_t **DOs;

/* getField takes *c, a pointer to beginning of string;
   char sep, a character seperator between fields
   **result, a ptr to pointer of the resulting string
   returns pointer to beginning of next field or NULL if at end
*/
char *getField(char *c, char sep, char **result) {
    unsigned int bytes = 0;
    *result = c;

    if (!c) return NULL;

    while ( (*c != sep) && (*c != '\0') ) {
        c++;
        bytes++;
    }
    if (*c == '\0') return NULL;
    *c = '\0';
    c++;
    return (c);
}

/* Generic field parsing */
void parse(char *string) {
    char *c;
    char *res;

    c = string;
    while (c) {
        c = getField(c, ':', &res);
        printf("DField: %s<BR>\n", res);
    }
}

/* takes a data object string and returns a DO */
/* ?DO=index:table:interface */
data_obj_t *parseDO(char *string) {
    data_obj_t *DO = NULL;
    char *c;
    char *res;

    DO = (data_obj_t *) malloc(sizeof(data_obj_t));
	bzero(DO, sizeof(data_obj_t));
	// XXX - REB - fixed static malloc
	DO->table = (char *) malloc(64*sizeof(char));
	bzero(DO->table, 64*sizeof(char));
    c = string;
    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Data Object Input!\n");
    DO->index = atoi(res);
    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Data Object Input!\n");
	sprintf(DO->table, "%s", res);
    c = getField(c, ':', &res);
    DO->id = atoi(res);

    /* Bad input, go to generic error routine */
    if (c)
        plotbail("Bad Data Object Input!\n");

    return (DO);
}

/* parse modifiers in a LO */
void parseLOmod(char *string, line_obj_t **LOptr) {
    char *c;
    char *v;
    char *res;
    line_obj_t *LO = *LOptr;

    LO->aggr = FALSE;
    LO->percentile = 0;
    LO->filled = FALSE;
    LO->factor = 1;
    c = string;
    while (c) {
        c = getField(c, ',', &res);
        if (strcasestr(res, "aggr")) {
            LO->aggr = TRUE;
        } else if (strcasestr(res, "percent")) {
            v = res;
            v = getField(v, '=', &res);
            v = getField(v, '=', &res);
            LO->percentile = atoi(res);
        } else if (strcasestr(res, "filled")) {
            LO->filled = TRUE;
        } else if (strcasestr(res, "factor")) {
            v = res;
            v = getField(v, '=', &res);
            v = getField(v, '=', &res);
            LO->factor = atoi(res);
        }
    }
}

/* parse modifiers in a PO */
void parsePOmod(char *string, plot_obj_t **POptr) {
    char *c;
    char *v;
    char *res;
    plot_obj_t *PO = *POptr;
    
    c = string;
    while (c) {
        c = getField(c, ',', &res);
        if (strcasestr(res, "scalex")) {
            PO->scalex = TRUE;
        } else if (strcasestr(res, "scaley")) {
            PO->scaley = TRUE;
        } else if (strcasestr(res, "gauge")) {
            PO->gauge = TRUE;
        } else if (strcasestr(res, "impulse")) {
            PO->impulse = TRUE;
        } else if (strcasestr(res, "units")) {
            v = res;
            v = getField(v, '=', &res);
            v = getField(v, '=', &res);
            PO->units = malloc(strlen(res)+1);
            strncpy(PO->units, res, strlen(res));
        }
    }
}

/* parse the DO in a LO */
void parseLODO(char *string, line_obj_t **LOptr) {
    char *c;
    char *res;
    data_obj_list_t *DOLlast = NULL;
    data_obj_list_t *DOL = NULL;
    line_obj_t *LO = *LOptr;
    int DOindex = 0;

    c = string;
    while (c) {
        c = getField(c, ',', &res);
        DOindex = atoi(res);

        /* Create a data object list (DOL) element */
        DOL = (data_obj_list_t *) malloc(sizeof(data_obj_list_t));
        bzero(DOL, sizeof(data_obj_list_t));
        DOL->DO = DOs[DOindex];
        if (DOL->DO == NULL)
            plotbail("No such Data Object for given Line Object!");
        DOL->next = NULL;

        if (LO->DO_list == NULL) {
            LO->DO_list = DOL;
            DOLlast = DOL;
        } else {
            DOLlast->next = DOL;
            DOLlast = DOL;
        }
		LO->DO_count++;
    }
}

/* takes a line object string and returns a LO */
/* ?LO=index:data:modifiers:appearance:label */
line_obj_t *parseLO(char *string) {
    line_obj_t *LO = NULL;
    char *c;
    char *res;
    int lengendLen = 0;

    LO = (line_obj_t *) malloc(sizeof(line_obj_t));
	bzero(LO, sizeof(line_obj_t));
	LO->label = (char *) malloc(LEGEND_MAX_LEN * sizeof(char));
    bzero(LO->label, LEGEND_MAX_LEN * sizeof(char));
    LO->DO_list = NULL;

    c = string;
    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Line Object Input!\n");
    LO->index = atoi(res);
    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Line Object Input!\n");
    parseLODO(res, &LO);
    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Line Object Input!\n");
    parseLOmod(res, &LO);
    c = getField(c, ':', &res);
    lengendLen = strlen(res)+1; 
    if (strlen(res) > LEGEND_MAX_LEN)
        lengendLen = LEGEND_MAX_LEN;
    snprintf(LO->label, lengendLen, "%s", res);

    /* Bad input, go to generic error routine */
    if (c)
        plotbail("Bad Line Object Input!\n");

    return (LO);
}

/* takes a plot object string and returns a PO */
/* ?PO=title:xsize:ysize:scalex:scaley:begin:end */
plot_obj_t *parsePO(plot_obj_t *PO, char *string) {
    char *c;
    char *res;

    c = string;
    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Plot Object Input #1!\n");
    PO->title = res;

    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Plot Object Input #2!\n");
    PO->image.xplot_area = atoi(res);

    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Plot Object Input #3!\n");
    PO->image.yplot_area = atoi(res);

    if ( (PO->image.xplot_area > XPLOT_AREA_MAX) || (PO->image.yplot_area > YPLOT_AREA_MAX) )
        plotbail("Bad Plot Area!\n");

    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Plot Object Input #4!\n");
    parsePOmod(res, &PO);

    c = getField(c, ':', &res);
    if (!c) plotbail("Bad Plot Object Input #5!\n");
    PO->range.begin = atol(res);

    c = getField(c, ':', &res);
    PO->range.end = atol(res);

    if (PO->range.end <= PO->range.begin)
        plotbail("Bad Plot Range!\n");

    /* Bad input, go to generic error routine */
    if (c)
        plotbail("Bad Plot Object Input! foo\n");

    return (PO);
}
