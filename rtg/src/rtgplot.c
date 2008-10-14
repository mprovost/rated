/****************************************************************************
   Program:     $Id: rtgplot.c,v 1.61 2008/01/19 03:01:32 btoneill Exp $
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Description: RTG traffic grapher
                Utilizes cgilib (http://www.infodrom.org/projects/cgilib/)
                for the CGI interface.
****************************************************************************/

/* 0.8 NOTE!!
  You need to call rtgplot.cgi with NEW arguments in version 0.8.
  Until I document it, here is an example:

http://momo.lcs.mit.edu/rtg/test/rtgplot.cgi?PO=Custom%20SuperDuper%20Plot:460:500:1:0:1106283600:1106369999&DO=1:ifinOctets_2:4
&DO=2:ifInOctets_5:8&DO=43:ifInOctets_3:1&DO=4:ifOutOctets_1:9&LO=1:1,2:aggr,percentile=55,filled:aggrline&LO=2:43:factor=8:aline
&LO=43:4:factor=3,percent=92:third

*/

/*
  We define the image as follows:

  +--------------------------------------------------------------+   ^
  |                                  BORDER_T                    |   |  
  |            +---------------------------------------------+   |   |  
  |            |                    ^                        |   |   | Y
  |            |                    |                        | B |   | I
  |            |                    |                        | O |   | M
  |            |                    |                        | R |   | G
  |            |                    |                        | D |   | _
  |<-BORDER_L->|               YPLOT_AREA                    | E |   | A
  |            |                    |                        | R |   | R
  |            |                    |                        | _ |   | E
  |            |                    |                        | R |   | A
  |            |                    |                        |   |   |
  |            |                    |                        |   |   |
  |            | <----------XPLOT_AREA---------------------> |   |   |
  |            +---------------------------------------------+   |   |
  |                                  BORDER_B                    |   |
  +--------------------------------------------------------------+   ^

  <-----------------------XIMG_AREA------------------------------>   

*/

/*
 XXX - rewrite this XXX 
  REB - rtgplot in a nutshell:
	1. Grab command line or HTTP arguments
	2. Read RTG configuration rtg.conf
	3. Create empty graph with arrows, borders, etc
	4. Populate a data_t linked list for each interface
	5. Calculate a rate_t rate struct for each data linked list that
	   defines total, max, avg, current rates
	6. Normalize each line in relation to other lines, graph size, etc
	7. Plot each line and legend
	8. Plot X and Y scales
	9. Write out graph 

  There are a bunch of subtleties beyond this that we try to comment.
*/

#include "rtg.h"
#include "rtgplot.h"
#include "cgi.h"
#include "rtgdbi.h"

/* XXX - REB - Blow away */
int HTMLOUT=0;

config_t config;
config_t *set = &config;
int lock;
int waiting;
char config_paths[CONFIG_PATHS][BUFSIZE];

/* dfp is a debug file pointer.  Points to stderr unless debug=level is set */
FILE *dfp = NULL;

/* RTG Plots are comprised of DOs, LOs and a PO */
data_obj_t **DOs;       // array of pointers to data objects
line_obj_t **LOs;       // array of pointers to line objects
plot_obj_t *PO = NULL;  // pointer to plot object
count_t count;

/* cgiHeader() wrapper */
void cgi_header() {
	static int wrote = FALSE;

	if (!wrote) {
		cgiHeader();
		wrote=TRUE;
	}
}

/* XXX - REB - change this to display plot error text in image */
void plotbail(char *s) {
	cgi_header();
	printf("rtgplot %s fatal: %s<BR>\n",
		VERSION, s);
	exit(-1);
}

static inline void plotdebug(char *fmt, ...) {
    va_list argp;
if (HTMLOUT) {
    cgi_header();
    printf("<BR><STRONG>DEBUG: </STRONG>\n");
    va_start(argp, fmt);
    vprintf(fmt, argp);
    va_end(argp);
    printf("\n");
}
}

/* A wrapper around libcgi's cgiGetValue to count the number
   of occurences of a passed var */
int cgiGetValueCount(s_cgi **cgiArg, char *value) {
    unsigned int count = 0;
    char *temp = NULL;
    int i = 0;
    int n = 0;

    if ((temp = cgiGetValue(cgiArg, value))) {
        count++;
        n = strlen(temp); 
        for (i=0;i<n;i++) {
            if (*temp == '\0') break;
            if (*temp == '\n') count++;
            temp++;
        }
    }
    return count;
}

int main(int argc, char **argv) {
	gdImagePtr      img;
	color_t        *colors = NULL;
        arguments_t     arguments;
	char            query[BUFSIZE];
	char			intname[BUFSIZE];
	int             i, j;
	int             offset = 0;
	float			interfaceSpeed = 0.0;
	data_obj_list_t	*DOLptr;
	data_t			*dataPtr;

	/* debug file pointer */
	dfp = stderr;

	/* Initialize plot */
	PO = (plot_obj_t *) malloc(sizeof(plot_obj_t));
	bzero(PO, sizeof(plot_obj_t));
	sizeDefaults(PO);
	bzero(&arguments, sizeof(arguments_t));
	config_defaults(set);

	/* Called via a CGI */
	if (getenv("SERVER_NAME")) 
		parseWeb(&arguments, PO);
	/* Check argument count */
	else if (argc < 2)
		usage(argv[0]);
	/* Called from the command line with arguments */
	else
		parseCmdLine(argc, argv, &arguments, PO);

    if (PO->units == NULL) {
        PO->units = malloc(sizeof(DEFAULT_UNITS));
        strncpy(PO->units, DEFAULT_UNITS, sizeof(DEFAULT_UNITS));
    }
	PO->range.dataBegin = PO->range.end;
	sizeImage(PO, count.LOs);

	/* Read configuration file to establish local environment */
	if (arguments.conf_file) {
		if ((read_rtg_config(arguments.conf_file, set)) < 0) 
			fatalfile(dfp, "Could not read config file: %s\n", arguments.conf_file);
	} else {
		arguments.conf_file = malloc(BUFSIZE);
		for (i = 0; i < CONFIG_PATHS; i++) {
			snprintf(arguments.conf_file, BUFSIZE, "%s%s", config_paths[i], DEFAULT_CONF_FILE);
			if (read_rtg_config(arguments.conf_file, set) >= 0) {
				break;
			}
			if (i == CONFIG_PATHS - 1) {
				snprintf(arguments.conf_file, BUFSIZE, "%s%s", config_paths[0], DEFAULT_CONF_FILE);
				if ((write_rtg_config(arguments.conf_file, set)) < 0) 
					fatalfile(dfp, "Couldn't write config file.\n");
			}
		}
	}

        if (!(db_init(set))) {
            printf("Ack! database error!\n");
            fatal("** foo Database error - check configuration.\n");
        }
        /* connect to the database */
        if (!(db_connect(set))) {
            fatal("server not responding.\n");
        }

    /* Initialize the graph */
	create_graph(&img, PO);
	init_colors(&img, &colors);
	draw_grid(&img, PO);

	/* set xoffset to something random, but suitably large */
	PO->xoffset = 2000000000;

	/* If we're y-scaling the plot to max interface speed */
	if (PO->scaley) {
		for (i=0;i<=count.DOs;i++) {
			if (DOs[i]) {
				interfaceSpeed = (float) intSpeed(DOs[i]->id);
				if (interfaceSpeed > PO->ymax)
					PO->ymax = interfaceSpeed;
			}
		}
	}

	if (HTMLOUT) {
		checkCount(&count);
		printPOs();
		printDOs();
		printLOs();
	}

/* XXX - REB - PGSQL timezone brokeness */
/*
timezone('EST',(date('epoch') + interval '%ld s')) AND dtime<=
snprintf(query, sizeof(query), 
"SELECT counter, date_part('epoch', dtime) FROM %s WHERE dtime>\
(date('epoch') + interval '%ld s' - interval '14400 s') AND dtime<=\
(date('epoch') + interval '%ld s' - interval '14400 s') AND id=%d ORDER BY dtime", 
arguments.table[i], graph.range.begin, graph.range.end, arguments.iid[j]);
*/

	/* Populate the data linked lists */ 
	for (i=0;i<=count.DOs;i++) {
		if (DOs[i]) {
			if (HTMLOUT) 
				printf("Doing DO %d <br>\n", i);
/*			snprintf(query, sizeof(query), "SELECT counter, dtime FROM %s WHERE dtime>=%ld AND dtime<=%ld AND id=%d ORDER BY dtime",  */
                        snprintf(query, sizeof(query), "SELECT counter, UNIX_TIMESTAMP(dtime) FROM %s WHERE dtime>FROM_UNIXTIME(%ld) AND dtime <=FROM_UNIXTIME(%ld) AND id=%d ORDER BY dtime",
				DOs[i]->table, PO->range.begin, PO->range.end, DOs[i]->id);
			if (populate(query, (DOs[i])) < 0) {
				//printf("problem populating(), recreating query <br>\n");
				// XXX - REB - this doesn't make sense to me looking at it now 
				snprintf(query, sizeof(query), "SELECT counter, dtime FROM %s WHERE id=%d AND rownum < 2 ORDER BY dtime DESC",
					DOs[i]->table, DOs[i]->id);
				/* Recreate the query to get the last point in the DB.  
					Then recall populate. */
				if (populate(query, (DOs[i])) < 0) {
					plotdebug("No data to populate() for table: %s int: %d\n", DOs[i]->table, DOs[i]->id);
				}
			} 
		}
	}

	/* Delete empty DOs and LOs */
	clearEmpties();

	/* get graph stats */
	for (i=0;i<=count.DOs;i++) {
		if (DOs[i]) {
			/* we're plotting impulses or gauge */
			if (PO->impulse || PO->gauge) 
				calculate_total(&(DOs[i]->data), &(DOs[i]->rate_stats)); 
			else 
				calculate_rate(&(DOs[i]->data), &(DOs[i]->rate_stats)); 
		}
	}

	
	if (HTMLOUT) {
		printf("Rate Stats:<br>\n");
    	printf("<blockquote>\n");
		for (i=0;i<=count.DOs;i++) {
			if (DOs[i]) {
				dump_rate_stats(&(DOs[i]->rate_stats));
				printf("<br>\n");
			}
		}
    	printf("</blockquote>\n");
	}


	/* Do the DO/LO aggregation */
	for (i=0;i<=count.LOs;i++) {
		if (LOs[i]) {
			if (HTMLOUT) 
				printf("doing LO %d (%s) with (%d DOs)<br>\n", i, LOs[i]->label, LOs[i]->DO_count);
			/* XXX - we're always using DO_aggr, change variable */
			LOs[i]->DO_aggr = (data_obj_t *) malloc(sizeof(data_obj_t));
			/* XXX - REB - fix this static 64B malloc to accomodate different size tables throughout code */
			LOs[i]->DO_aggr->table = (char *) malloc(64*sizeof(char));
			copyDO(LOs[i]->DO_aggr, LOs[i]->DO_list->DO);
			if ( (LOs[i]->DO_count > 1) && (LOs[i]->aggr == TRUE) ) {
				DOLptr = LOs[i]->DO_list;
				// aggregate with rest of DOs in LO
				DOLptr = DOLptr->next;
				while (DOLptr != NULL) {
					if (HTMLOUT) printf("aggregate with: DO[%d]<br>\n", DOLptr->DO->index);
					dataAggr(&(LOs[i]->DO_aggr->data), DOLptr->DO->data, &(LOs[i]->DO_aggr->rate_stats), 
						&(DOLptr->DO->rate_stats));
					DOLptr = DOLptr->next;
				}
			}
		}
	}

	/* Factor scaling */
	for (i=0;i<=count.LOs;i++) {
		if (LOs[i]) {
			rateFactor(&(LOs[i]->DO_aggr->rate_stats), LOs[i]->factor);
			/* Our graph time range is the largest of the ranges; set maximum X */
            if (PO->scalex == TRUE) {
			 /* Realign the start time to be consistent with
			 * actual DB data.  Use dataBegin, reset only if less
			 * than last dataBegin and larger than requested begin 
			 */
			    if (PO->range.dataBegin > LOs[i]->DO_aggr->dataBegin)
				    PO->range.dataBegin = LOs[i]->DO_aggr->dataBegin;
                if (PO->range.end < LOs[i]->DO_aggr->dataEnd)
                    PO->range.end = LOs[i]->DO_aggr->dataEnd;
			    if (PO->range.end - LOs[i]->DO_aggr->dataBegin > PO->xmax) 
				    PO->xmax = PO->range.end - LOs[i]->DO_aggr->dataBegin;
			    if (LOs[i]->DO_aggr->dataBegin < PO->xoffset)
				    PO->xoffset = LOs[i]->DO_aggr->dataBegin;
            } else {
                PO->range.dataBegin = PO->range.begin;
                PO->xmax = PO->range.end - PO->range.begin;
                PO->xoffset = PO->range.begin;
            }
			/* maximum Y value is largest of all line rates */
			if ( (PO->scaley == FALSE) && (LOs[i]->DO_aggr->rate_stats.max > PO->ymax) )
			/* Extend Y-Axis to prevent line from tracing top of YPLOT_AREA  */ 
				PO->ymax = (LOs[i]->DO_aggr->rate_stats.max) * 1.05;
		}
	}

	/* Normalize lines and created legends */
	for (i=0;i<=count.LOs;i++) {
		if (LOs[i]) {
			normalize(LOs[i]->DO_aggr->data, PO, LOs[i]->factor);
			LOs[i]->shade = colors->shade;
			plot_legend(&img, PO, LOs[i], offset);
			colors = colors->next;
			offset++;
		}
	}

	/* Plot filled lines and legends first */
	for (i=0;i<=count.LOs;i++) {
		if ( (LOs[i] != NULL) && (LOs[i]->filled == TRUE) ) 
			plot_line(&img, PO, LOs[i]);
	}

	/* Plot unfilled lines next */
	for (i=0;i<=count.LOs;i++) {
		if ( (LOs[i] != NULL) && (LOs[i]->filled == FALSE) ) 
			plot_line(&img, PO, LOs[i]);
	}

	/* Plot percentile lines if requested */
	for (i=0;i<=count.LOs;i++) {
		if (LOs[i] && LOs[i]->percentile > 0) {
			/* sort the newly created data_t list for the LO here */
			LOs[i]->DO_aggr->data = sort_data(LOs[i]->DO_aggr->data, FALSE, FALSE);
			dataPtr = return_Nth(LOs[i]->DO_aggr->data, count_data(LOs[i]->DO_aggr->data), LOs[i]->percentile);
			plot_Nth(&img, PO, dataPtr, LOs[i]->shade);
		}
	}
	

	if (HTMLOUT) {
		printf("<strong>Ready to plot:</strong>\n");
		printDOs();
		printLOs();
		printPOs();
	}

	/* Put finishing touches on graph and write it out */
	draw_border(&img, PO);
	draw_arrow(&img, PO);
	plot_scale(&img, PO);
	plot_labels(&img, PO);

	if (!HTMLOUT) {
		write_graph(&img, arguments.output_file);
	}

	/* Disconnect from the MySQL Database, exit. */
    db_disconnect(NULL);
    if (dfp != stderr) fclose(dfp);
    exit(0);
}


void dump_data(data_t *head) {
	data_t         *entry = NULL;
	int             i = 0;

	fprintf(dfp, "Dumping data:\n");
	entry = head;
	while (entry != NULL) {
		fprintf(dfp, "  [%d] Count: %lld TS: %ld Rate: %2.3f X: %d Y: %d [p: %p]\n",
		       ++i, entry->counter, entry->timestamp, entry->rate,
		       entry->x, entry->y, entry);
		entry = entry->next;
	}
}

int copyDO(data_obj_t *dst, data_obj_t *src) {
	unsigned long datapoints = src->datapoints;

	data_t *data = NULL;
	data_t *last = NULL;
	data_t *srcdata = src->data;
	if (HTMLOUT) 
		printf("malloc'ing %d bytes\n", datapoints * sizeof(data_t));
	data = (data_t *) malloc(datapoints * sizeof(data_t));
	dst->data = data;
	dst->index = src->index;
	dst->datapoints = datapoints;
	dst->dataBegin = src->dataBegin;
	dst->id = src->id;
	memcpy(&(dst->rate_stats), &(src->rate_stats), sizeof(rate_t));
	memcpy(dst->table, src->table, strlen(src->table));
	while (srcdata) {
		memcpy(data, srcdata, sizeof(data_t));
		if (last != NULL) 
			last->next = data;
		last = data;
		data++;
		srcdata = srcdata->next;
	}
}

/*
 * if we're doing impulses, we can't calculate a rate.  Instead, we just
 * populate and figure out the max values for plotting
 */
/* Snarf the MySQL data into a linked list of data_t's */
int populate(char *query, data_obj_t *DO) {
	data_t *new = NULL;
	data_t *last = NULL;
	data_t **data = &(DO->data);
        int stat;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Populating (%s).\n", __FUNCTION__);

	if (set->verbose >= DEBUG) 
		fprintf(dfp, "  Query String: %s\n", query);
       
        stat = db_populate(query,DO); 
        if ((new = (data_t *) malloc(sizeof(data_t))) == NULL) 
            debug(LOW, "  Fatal malloc error in populate.");
	
/*	while ((row = mysql_fetch_row(result))) {
		if ((new = (data_t *) malloc(sizeof(data_t))) == NULL) 
			fprintf(dfp, "  Fatal malloc error in populate.\n");
#ifdef HAVE_STRTOLL
		new->counter = strtoll(row[0], NULL, 0);
#else
		new->counter = strtol(row[0], NULL, 0);
#endif
		new->timestamp = atoi(row[1]);
		new->next = NULL;
		(DO->datapoints)++;
		if (new->counter > DO->counter_max)
			DO->counter_max = new->counter;
		if (*data != NULL) {
			last->next = new;
			last = new;
		} else {
			DO->dataBegin = new->timestamp;
			*data = new;
			last = new;
		}
	}
        */
	
        /* no data, go home */
	if (stat == FALSE) {
		if (set->verbose >= DEBUG)
			fprintf(dfp, "  No Data Points %ld.\n", DO->datapoints);
		return (-1);
	} else {
		if (set->verbose >= DEBUG)
			fprintf(dfp, "  %ld Data Points\n", DO->datapoints);
		/* record end time to be consistent with what's in the DB */
	}
	return (DO->datapoints);
}


void calculate_total(data_t **data, rate_t *rate) {
	data_t         *entry = NULL;
	int		num_samples = 0;
	float		ratetmp;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Calc total (%s).\n", __FUNCTION__);
	entry = *data;
	rate->total = 0;
	while (entry != NULL) {
		num_samples++;
		if (set->verbose >= DEBUG)
			fprintf(dfp, "  [TS: %ld][Counter: %lld]\n", entry->timestamp,
			       entry->counter);
		ratetmp = entry->counter;
		rate->total += ratetmp;
		rate->cur = ratetmp;
		if (ratetmp > rate->max)
			rate->max = ratetmp;
		entry->rate = ratetmp;
		entry = entry->next;
	}
	if (num_samples > 0) {
		rate->avg = rate->total / num_samples;
	} else {
		rate->avg = 0;
	}
}


void calculate_rate(data_t **data, rate_t *rate_stats) {
	data_t         *entry = NULL;
	float           rate = 0.0;
	float           last_rate = 0.0;
	int             sample_secs = 0;
	int             last_sample_secs = 0;
	int             num_rate_samples = 0;
	unsigned long	start_time;
	int             i;

        debug(HIGH, "Calc rate (%s).\n", __FUNCTION__);

	entry = *data;
	if (entry == NULL)
		return;

	start_time = entry->timestamp;
	while (entry != NULL) {
		rate_stats->total += entry->counter;
		sample_secs = entry->timestamp;
		if (last_sample_secs != 0) {
			num_rate_samples++;
			rate = (float)entry->counter / (sample_secs - last_sample_secs);
			/*
			 * Compensate for polling slop.
			 * set.highskewslop and lowskewslop are configurable.
			 * If two values are too far or too close together,
			 * in time, then set rate to last_rate.
			 */
			if (sample_secs - last_sample_secs > set->highskewslop * set->interval) {
				if (set->verbose >= LOW) {
					fprintf(dfp, "***Poll skew [elapsed secs=%d] [interval = %d] [slop = %2.2f]\n",
					       sample_secs - last_sample_secs, set->interval, set->highskewslop);
				}
				rate = last_rate;
			}
			if (sample_secs - last_sample_secs < set->lowskewslop * set->interval) {
				if (set->verbose >= LOW) {
					fprintf(dfp, "***Poll skew [elapsed secs=%d] [interval = %d] [slop = %2.2f]\n",
					       sample_secs - last_sample_secs, set->interval, set->lowskewslop);
				}
				rate = last_rate;
			}
			if (set->verbose >= DEBUG)
				fprintf(dfp, "  [row0=%lld][row1=%d][elapsed secs=%d][rate=%2.3f bps]\n", entry->counter, sample_secs, sample_secs - last_sample_secs, rate);
			if (rate < 0 && set->verbose >= LOW)
				fprintf(dfp, "  ***Err: Negative rate!\n");
			if (rate > rate_stats->max)
				rate_stats->max = rate;
		}
		entry->rate = rate;
		last_sample_secs = sample_secs;
		last_rate = rate;
		entry = entry->next;
	}
	rate_stats->cur = rate;
    rate_stats->avg = (float)rate_stats->total / (sample_secs - start_time);

	/*
	 * A rate is calcuated as a function of time which requires at least
	 * two data points.  Because of this our first data point will always
	 * be zero, so set the first data point the same as the second
	 * datapoint
	 */
	if (num_rate_samples > 1) {
		(*data)->rate = (*data)->next->rate;
	}
}


void dataAggr(data_t **aggr, data_t *head, rate_t *aggr_rate, 
				rate_t *rate) {
	float last_aggr_rate;
	float last_head_rate;

    if (set->verbose >= HIGH)
        fprintf(dfp, "Aggregate (%s).\n", __FUNCTION__);

	last_aggr_rate = (*aggr)->rate;
	last_head_rate = head->rate;
    while ( (head != NULL) && (*aggr != NULL) ) {
        if (head->timestamp < (*aggr)->timestamp) {
			if (set->verbose >= DEBUG) fprintf(dfp, ".h");
            last_aggr_rate = (*aggr)->rate;
            (*aggr)->rate = last_head_rate + (*aggr)->rate;
			if ((*aggr)->rate > aggr_rate->max) 
				aggr_rate->max = (*aggr)->rate;
			head = head->next;
		}
        else if (head->timestamp > (*aggr)->timestamp) {
			if (set->verbose >= DEBUG) fprintf(dfp, ".a");
            last_head_rate = head->rate;
            (*aggr)->rate = last_aggr_rate + head->rate;
			if ((*aggr)->rate > aggr_rate->max) 
				aggr_rate->max = (*aggr)->rate;
			aggr = &((*aggr)->next);
		}
        else if (head->timestamp == (*aggr)->timestamp) {
			if (set->verbose >= DEBUG) fprintf(dfp, ".+");
            last_aggr_rate = (*aggr)->rate;
            last_head_rate = head->rate;
			(*aggr)->rate = (*aggr)->rate + head->rate;
			if ((*aggr)->rate > aggr_rate->max) 
				aggr_rate->max = (*aggr)->rate;
			aggr_rate->cur = (*aggr)->rate;
            head = head->next;
			aggr = &((*aggr)->next);
        }
    }
	(aggr_rate->avg)+= rate->avg;
	(aggr_rate->total)+= rate->total; 
	return;
}


void normalize(data_t *head, plot_obj_t *graph, int factor) {
	data_t         *entry = NULL;
	long            time_offset = 0;
	float           pixels_per_sec = 0.0;
	float           pixels_per_bit = 0.0;
	int             last_x_pixel = 0;
	int             x_pixel = 0;
	int             y_pixel = 0;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Normalize (%s).\n", __FUNCTION__);

	if (graph->ymax > 0)
		pixels_per_bit = (float)graph->image.yplot_area / graph->ymax;
	if (graph->xmax > 0)
		pixels_per_sec = (float)graph->image.xplot_area / graph->xmax;

	if (graph->xmax > 0 && graph->xmax <= HOUR)
		graph->xunits = HOUR;
	else if (graph->xmax > HOUR && graph->xmax <= HOUR * DAY)
		graph->xunits = DAY;
	else if (graph->xmax > HOUR * DAY && graph->xmax <= HOUR * DAY * WEEK)
		graph->xunits = WEEK;
	else if (graph->xmax > HOUR * DAY * WEEK)
		graph->xunits = MONTH;

	if (graph->ymax > 0 && graph->ymax <= KILO)
		graph->yunits = 1;
	else if (graph->ymax > KILO && graph->ymax <= MEGA)
		graph->yunits = KILO;
	else if (graph->ymax > MEGA && graph->ymax <= GIGA)
		graph->yunits = MEGA;
	else if (graph->ymax > GIGA)
		graph->yunits = GIGA;
	else
		graph->yunits = -1;

	entry = head;
	time_offset = graph->range.dataBegin;
	if (set->verbose >= HIGH) {
		fprintf(dfp, "  X-Max = %d Y-Max = %2.3f\n", graph->xmax, graph->ymax);
		fprintf(dfp, "  X-units = %d Y-units = %lld\n", graph->xunits, graph->yunits);
		fprintf(dfp, "  XPixels/Sec = %2.6f\n", pixels_per_sec);
		fprintf(dfp, "  YPixels/Bit = %2.10f\n", pixels_per_bit);
		fprintf(dfp, "  Timeoffset = %ld\n", time_offset);
	}
	while (entry != NULL) {
		x_pixel = (int)((entry->timestamp - time_offset) * pixels_per_sec + .5);
		y_pixel = (int)((entry->rate * factor * pixels_per_bit) + .5);
		entry->x = x_pixel;
		entry->y = y_pixel;
		if (x_pixel != last_x_pixel) {
			if (set->verbose >= DEBUG) {
				fprintf(dfp, "  TS=%ld T=%ld X=%d R=%2.3f RN=%2.5f Y=%d F=%d\n",
				       entry->timestamp, entry->timestamp - time_offset, x_pixel,
				       entry->rate, entry->rate / graph->yunits, y_pixel, factor);
			}
		}
		last_x_pixel = x_pixel;
		entry = entry->next;
	}

	return;
}


void usage(char *prog) {
	printf("rtgplot - RTG v%s\n", VERSION);
	printf("Usage: %s [-c file] [-o file] -d data_obj -l line_obj -p plot_obj [-vv]\n", prog);
	printf("    -d data object (DOid:table:iid)\n");
	printf("    -l line object (LOid:DOid,DOid,..,DOid:options:legend)\n");
	printf("    -p plot object (title:xsize:ysize:scalex:scaley:begin:end)\n");
	printf("    -c config file\n");
	printf("    -o output file, PNG format (default to stdout)\n");
	printf("    -v verbosity (can use multiple times)\n");
	exit(-1);
}


void dump_rate_stats(rate_t * rate) {
	fprintf(dfp, "[total = %lld][max = %2.3f][avg = %2.3f][cur = %2.3f]\n",
	       rate->total, rate->max, rate->avg, rate->cur);
}


char *units(float val, char *string) {
	if (val > TERA)
		snprintf(string, BUFSIZE, "%2.1f T", val / TERA);
	else if (val > GIGA)
		snprintf(string, BUFSIZE, "%2.1f G", val / GIGA);
	else if (val > MEGA)
		snprintf(string, BUFSIZE, "%2.1f M", val / MEGA);
	else if (val > KILO)
		snprintf(string, BUFSIZE, "%2.1f K", val / KILO);
	else
		snprintf(string, BUFSIZE, "%2.2f", val);
	return (string);
}


void plot_legend(gdImagePtr *img, plot_obj_t *graph, line_obj_t *LO, int offset) {
	char            total[BUFSIZE];
	char            max[BUFSIZE];
	char            avg[BUFSIZE];
	char            cur[BUFSIZE];
	char            string[BUFSIZE];
    char            interface[BUFSIZE];
	char			*cp;
    rate_t          *rate;
	int				spaces;
	int             i;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Plotting legend (%s).\n", __FUNCTION__);

    rate = &(LO->DO_aggr->rate_stats);

	gdImageFilledRectangle(*img, BORDER_L + LEGENDOFFSET,
			     BORDER_T + graph->image.yplot_area + 37 + 10 * offset,
	BORDER_L + LEGENDOFFSET + 7, BORDER_T + graph->image.yplot_area + 44 + 10 * offset, LO->shade);
	gdImageRectangle(*img, BORDER_L + LEGENDOFFSET, BORDER_T + graph->image.yplot_area + 37 + 10 * offset,
	       BORDER_L + LEGENDOFFSET + 7, BORDER_T + graph->image.yplot_area + 44 + 10 * offset,
			 std_colors[black]);

	/* Either chop interface name or pad with spaces to make all same length */
    sprintf(interface, "%s", LO->label);
	if (strlen(interface) > LEGEND_MAX_LEN) {
		interface[LEGEND_MAX_LEN] = '\0';
	} else {
		spaces = LEGEND_MAX_LEN - strlen(interface);
		cp = &interface[strlen(interface)];
		for (i = 0; i<spaces; i++) {
			*cp = ' ';
			cp++;
		}
		*cp = '\0';
	}

	if (graph->gauge) {
		snprintf(string, sizeof(string), "%s Max: %7s %s Avg: %7s %s Cur: %7s %s",
			interface,
			units(rate->max, max), graph->units,
			units(rate->avg, avg), graph->units,
			units(rate->cur, cur), graph->units);
	} else if (graph->impulse) {
		snprintf(string, sizeof(string), "%s Total: %lld Max: %02.1f",
			interface, rate->total, (float)rate->max);
	} else {
		snprintf(string, sizeof(string), "%s Max: %7s%s Avg: %7s%s Cur: %7s%s [%7s]",
			interface,
			units(rate->max, max), graph->units,
			units(rate->avg, avg), graph->units,
			units(rate->cur, cur), graph->units,
			units((float)rate->total, total));
	}
	gdImageString(*img, gdFontSmall, BORDER_L + LEGENDOFFSET + 10,
		      BORDER_T + graph->image.yplot_area + 33 + (10 * offset), string, std_colors[black]);
}


void plot_line(gdImagePtr *img, plot_obj_t *graph, line_obj_t *LO) {
	data_t         *entry = NULL;
    data_t         *head = NULL;
	float			pixels_per_sec = 0.0;
	time_t			now;
	int             lastx = 0;
	int             lasty = 0;
	int             skip = 0;
	int				datapoints = 0;
	int				xplot_area = 0;
	int				now_pixel = 0;
	gdPoint			points[4];

    head = LO->DO_aggr->data;
	if (head == NULL) {
		/* XXX */
		printf("WARNING: head null\n");
		return;
	}
	entry = head;
	lasty = entry->y;
	datapoints = graph->range.datapoints;
	xplot_area = graph->image.xplot_area;
	/* The skip integer allows data points to be skipped when the scale
	   of the graph is such that data points overlap */
	skip = (int)datapoints / xplot_area;
	
	/* This nonsense is so that we don't have to depend on ceil/floor functions in math.h */
	if (datapoints % xplot_area != 0)
		skip++;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Plotting line.  Skip = %d (%s).\n", skip, __FUNCTION__);

	while (entry != NULL) {
		if (entry->x != (lastx + skip + 2)) {
			if (entry->y != 0 || lasty != 0) {
				if (graph->impulse) {
					gdImageLine(*img, 
                        entry->x + BORDER_L, 
                        graph->image.yimg_area - graph->image.border_b,
						entry->x + BORDER_L, 
                        graph->image.yimg_area - graph->image.border_b - entry->y, 
                        LO->shade);
				} else {
					if (set->verbose >= DEBUG) {
						fprintf(dfp, "  Plotting from (%d,%d) to (%d,%d) ",
						       lastx + BORDER_L, graph->image.yimg_area - graph->image.border_b - lasty,
						       entry->x + BORDER_L, graph->image.yimg_area - graph->image.border_b - entry->y);
						fprintf(dfp, "[entry->x/y = %d/%d] [lastx/y = %d/%d]\n",
						       entry->x, entry->y, lastx, lasty);
					}
					if (LO->filled) {
						points[0].x = lastx + BORDER_L;
						points[0].y = graph->image.yimg_area - graph->image.border_b;
						points[1].x = lastx + BORDER_L;
						points[1].y = graph->image.yimg_area - graph->image.border_b - lasty;
						points[2].x = entry->x + BORDER_L;
						points[2].y = graph->image.yimg_area - graph->image.border_b - entry->y;
						points[3].x = entry->x + BORDER_L;
						points[3].y = graph->image.yimg_area - graph->image.border_b;
						gdImageFilledPolygon(*img, points, 4, LO->shade);
					} else {
						gdImageLine(*img, 
                            lastx + BORDER_L,
						    graph->image.yimg_area - graph->image.border_b - lasty, 
                            entry->x + BORDER_L,
						    graph->image.yimg_area - graph->image.border_b - entry->y, 
                            LO->shade);
					}
				} /* if impulses */
				lasty = entry->y;
			}
			lastx = entry->x;
		}		/* if not skipping */
		entry = entry->next;
	}			/* while */

	/* If we're not aligning the time x-axis to the values in the
			database, make sure we're current up to time(now) */
	pixels_per_sec = (float)graph->image.xplot_area / graph->xmax;

	/* Find the x-pixel of the current time */
	time(&now);
	now_pixel = (int)((now - graph->range.dataBegin) * pixels_per_sec + .5);

	/* Only extend plot if now is not in plot future */
/*	if ((now_pixel < BORDER_L + graph->image.xplot_area) &&
	    (!graph->impulse)) {
*/
	if ( now_pixel < BORDER_L + graph->image.xplot_area ) {
		if (set->verbose >= HIGH) 
			fprintf(dfp, "Extending plot to current time %ld (x-pixel %d)\n", now, now_pixel);
		gdImageLine(*img, lastx + BORDER_L,
			graph->image.yimg_area - graph->image.border_b - lasty, now_pixel + BORDER_L,
			graph->image.yimg_area - graph->image.border_b - lasty, LO->shade);
	} else {
		if (set->verbose >= HIGH) 
			fprintf(dfp, "Current time %ld (x-pixel %d) extends past plot end.\n", now, now_pixel);
	}

	return;
}


/* Plot a Nth percentile line across the graph at the element nth points to */
void plot_Nth(gdImagePtr *img, plot_obj_t *graph, data_t *nth, int color) {
	int styleDashed[3];
	int xplot_area;
	int yplot_area;

	xplot_area = graph->image.xplot_area;
	yplot_area = graph->image.yplot_area;

	/* Define dashed style */
	styleDashed[0] = color;
	styleDashed[1] = gdTransparent;
	styleDashed[2] = color;
	gdImageSetStyle(*img, styleDashed, 3);

	if (set->verbose >= HIGH)
		fprintf(dfp, "Plotting Nth Percentile (%s).\n", __FUNCTION__);
		gdImageLine(*img, BORDER_L,
			graph->image.yimg_area - graph->image.border_b - nth->y, 
			BORDER_L + graph->image.xplot_area,
			graph->image.yimg_area - graph->image.border_b - nth->y, 
			gdStyled);
	return;
}


void plot_scale(gdImagePtr *img, plot_obj_t *graph) {
	struct tm *thetime;
	float pixels_per_sec = 0.0;
	float pixels_per_bit = 0.0;
	char string[BUFSIZE];
	int i;
	int last_day = 0;
	int last_month = 0;
	time_t seconds;
	float rate = 0.0;
	float plot_rate = 0.0;
    float days = 0;
    float skip = 0;
    int coeff;
    int styleDottedLight[3], styleDottedDark[3];
    float xticks= 0 ;
    char *dayofweek[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char *month[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

	if (set->verbose >= HIGH)
		fprintf(dfp, "Plotting scale (%s).\n", __FUNCTION__);
	pixels_per_sec = (float)graph->image.xplot_area / graph->xmax;
	pixels_per_bit = (float)graph->image.yplot_area / graph->ymax;
    days = (int)graph->xmax / (60 * 60 * 24);

    styleDottedLight[0] = std_colors[light];
    styleDottedDark[0] = std_colors[black];
    styleDottedLight[1] = styleDottedDark[1] = gdTransparent;
    styleDottedLight[2] = styleDottedDark[2] = gdTransparent;
    /* Draw X-axis time labels */
    if (days < 2.1) {
        // calculate minute between the start date and the o'clock date
        if ( (graph->xoffset-((graph->xoffset/3600)*3600)) != 0 ) {
            skip=(3600-(graph->xoffset-((graph->xoffset/3600)*3600)))*pixels_per_sec;
        }
        // calculate the number of vertical lines
        if ( graph->xmax <= 86400 ) 
            coeff=1800;
        else
            coeff=3600;

        xticks=((graph->xmax-(skip/pixels_per_sec))) / coeff;

        if ( skip == 0 )
            seconds = graph->xoffset;
        else 
            seconds = 3600 + graph->xoffset-((graph->xoffset-((graph->xoffset/3600)*3600)));

        if ( (graph->xoffset-((graph->xoffset/3600)*3600)) < (30*60) && skip > 0 
                  && ((graph->image.xplot_area-skip)/xticks)< skip ) {
            gdImageSetStyle(*img, styleDottedLight, 3);
            gdImageLine(*img, skip -((graph->image.xplot_area-skip))/xticks + BORDER_L, BORDER_T+1,
                    skip -((graph->image.xplot_area-skip))/xticks + BORDER_L,
                    BORDER_T + graph->image.yplot_area -1,gdStyled);
        }
        for (i = 0 ; i < xticks ; i++ ) {
            if (i % 4 == 0 ) {
                thetime = localtime(&seconds);
                snprintf(string, sizeof(string),"%02d:%02d",thetime->tm_hour, thetime->tm_min);
                gdImageString(*img, gdFontSmall,(i*(graph->image.xplot_area-skip))/xticks +skip+ BORDER_L - 15,
                          BORDER_T + graph->image.yplot_area + 5, string,std_colors[black]);
                seconds += coeff*4;
                gdImageSetStyle(*img, styleDottedDark, 3);
            } else {
                gdImageSetStyle(*img, styleDottedLight, 3);
            }
            gdImageLine(*img,(i*(graph->image.xplot_area-skip))/xticks + skip + BORDER_L, BORDER_T+1, 
                    (i*(graph->image.xplot_area-skip))/(xticks) +skip + BORDER_L, 
                    BORDER_T + graph->image.yplot_area -1,gdStyled);
        }
    } else if (days < 14) {
        thetime=localtime(&graph->xoffset);
        if (((graph->xoffset+thetime->tm_gmtoff)-(((graph->xoffset+thetime->tm_gmtoff)/86400)*86400)) != 0 ) {
            skip=(86400-((graph->xoffset+thetime->tm_gmtoff)-(((graph->xoffset+thetime->tm_gmtoff)/86400)*86400)))*pixels_per_sec;
        }

        if ( graph->xmax <= 86400*8 )
            coeff=6;
        else 
            coeff=3;

        xticks=coeff*((graph->xmax-(skip/pixels_per_sec)))/86400;


        if ( skip == 0 ) 
            seconds = graph->xoffset;
        else 
            seconds = 86400+graph->xoffset-((graph->xoffset-((graph->xoffset/86400)*86400)));

        if ( skip > 0 ) {
            for ( i=0; i < coeff; i++) {
                if ((i*(graph->image.xplot_area-skip)/xticks)< skip) {
                    gdImageSetStyle(*img, styleDottedLight, 3);
                    gdImageLine(*img, skip - (i*(graph->image.xplot_area-skip))/xticks + BORDER_L, BORDER_T+1,
                           skip - (i*(graph->image.xplot_area-skip))/xticks + BORDER_L,
                           BORDER_T + graph->image.yplot_area -1, gdStyled);
                }
            }
        }

        for (i = 0 ; i < xticks ; i++ ) {
            if (i % coeff == 0 ) {
                seconds += 86400;
                thetime = localtime(&seconds);
		if (set->verbose >= HIGH)
			fprintf(dfp, "Labeling %d %d %d:%d:%d as %s.\n", 
			 seconds,thetime->tm_wday,
			 thetime->tm_hour,thetime->tm_min,thetime->tm_sec,
			 dayofweek[thetime->tm_wday]);
                snprintf(string, sizeof(string),"%s",dayofweek[thetime->tm_wday]);
                gdImageString(*img, gdFontSmall,(coeff*(graph->image.xplot_area-skip))/xticks/2 +(i*(graph->image.xplot_area-skip))/xticks + skip + BORDER_L -7 ,
                    BORDER_T + graph->image.yplot_area +5, string, std_colors[black]);
                gdImageSetStyle(*img, styleDottedDark, 3);
            } else {
                gdImageSetStyle(*img, styleDottedLight, 3);
            }
            gdImageLine(*img,(i*(graph->image.xplot_area-skip))/xticks + skip + BORDER_L, BORDER_T+1,
                    (i*(graph->image.xplot_area-skip))/(xticks)+ skip + BORDER_L,
                    BORDER_T + graph->image.yplot_area -1,gdStyled);
        }
    } else if (days < 32) {
        thetime=localtime(&graph->xoffset);
        skip=(604800-((graph->xoffset+thetime->tm_gmtoff)-(((graph->xoffset+thetime->tm_gmtoff)/604800)*604800)))*pixels_per_sec;
        if ( graph->xmax <= 604800*7 ) {
            coeff=7;
        } else {
            coeff=7;
        }
        xticks=coeff*((graph->xmax-(skip/pixels_per_sec)))/604800;

        if ( skip == 0 ) {
            seconds = graph->xoffset-259200;
        } else {
            seconds =259200+604800+graph->xoffset-((graph->xoffset-((graph->xoffset/604800)*604800)));
        }
        if ( skip > 0 ) {
            for ( i=0; i < coeff; i++) {
                if ((i*(graph->image.xplot_area-skip)/xticks)< skip ) {
                    if( i-1 % coeff == 3 ) {
                        thetime = localtime(&seconds);
                        snprintf(string, sizeof(string),"W%d",thetime->tm_yday/7+1);
                        gdImageString(*img, gdFontSmall,(coeff*(graph->image.xplot_area-skip))/xticks/2 - (i*(graph->image.
                            xplot_area-skip))/xticks + skip+ BORDER_L -7 ,
                            BORDER_T + graph->image.yplot_area +5, string, std_colors[black]);
                        gdImageSetStyle(*img, styleDottedDark, 3);
                    } else {
                        gdImageSetStyle(*img, styleDottedLight, 3);
                    }
                    gdImageLine(*img, skip -(i*(graph->image.xplot_area-skip))/xticks + BORDER_L, BORDER_T+1,
                        skip -(i*(graph->image.xplot_area-skip))/xticks + BORDER_L,
                        BORDER_T + graph->image.yplot_area -1,gdStyled);
                }
            }
        }
        for (i = 0 ; i < xticks ; i++ ) {
            if ( i % coeff == 3 ) {
                thetime = localtime(&seconds);
                seconds += 604800;
                if ( (coeff*(graph->image.xplot_area-skip))/xticks/2 +(i*(graph->image.xplot_area-skip))/xticks + skip -7 <graph->image.xplot_area ) {
                    snprintf(string, sizeof(string),"W%d",thetime->tm_yday/7+2);
                    gdImageString(*img, gdFontSmall,(coeff*(graph->image.xplot_area-skip))/xticks/2 + (i*(graph->image.
                        xplot_area-skip))/xticks + skip +BORDER_L -7 ,
                        BORDER_T + graph->image.yplot_area + 5, string,std_colors[black]);
                }
                gdImageSetStyle(*img, styleDottedDark, 3);
            } else {
                gdImageSetStyle(*img, styleDottedLight, 3);
            }
            gdImageLine(*img, (i*(graph->image.xplot_area-skip))/xticks + skip + BORDER_L, BORDER_T+1,
                (i*(graph->image.xplot_area-skip))/(xticks) + skip + BORDER_L,
                BORDER_T + graph->image.yplot_area -1, gdStyled);
        }

    } else if (days < 370) {
        thetime=localtime(&graph->xoffset);
        thetime->tm_min=thetime->tm_hour=0;
        thetime->tm_mday=1;
        seconds=mktime(thetime);
        thetime = localtime(&seconds); 
        skip=((30*24*60*60)-(graph->xoffset-seconds))*pixels_per_sec;
        coeff=3;
        xticks=coeff*((graph->xmax-(skip/pixels_per_sec)))*12/(365*24*60*60);

        if ( skip == 0 ) {
            seconds = graph->xoffset;
        } else {
            thetime->tm_mon++;
            seconds=mktime(thetime);
            //seconds =2592000+graph->xoffset-((graph->xoffset-seconds));
        }
        if ( skip > 0 ) {
            for ( i=0; i < coeff; i++) {
                if ((i*(graph->image.xplot_area-skip)/xticks)< skip ) {
                    if( i % coeff == 2 ) {
                        thetime = localtime(&seconds);
                        if ( thetime->tm_mon == 0 ) {
                            snprintf(string, sizeof(string),"%s",month[11]);
                        } else {
                            snprintf(string, sizeof(string),"%s",month[thetime->tm_mon-1]);
                        }
                        gdImageString(*img, gdFontSmall,(coeff*(graph->image.xplot_area-skip))/xticks/4 - (i*(graph->image.
                            xplot_area-skip))/xticks + skip+ BORDER_L -7 ,
                            BORDER_T + graph->image.yplot_area + 5,string, std_colors[black]);
                      }
                      gdImageSetStyle(*img, styleDottedLight, 3);
                      gdImageLine(*img, skip -(i*(graph->image.xplot_area-skip))/xticks + BORDER_L, BORDER_T+1,
                        skip -(i*(graph->image.xplot_area-skip))/xticks + BORDER_L,
                        BORDER_T + graph->image.yplot_area -1,gdStyled);
                }
            }
        }
        for (i = 0 ; i < xticks ; i++ ) {
            if ( i % coeff == 0 ) {
                thetime = localtime(&seconds);
                if ( (coeff*(graph->image.xplot_area-skip))/xticks/2 +(i*(graph->image.xplot_area-skip))/xticks + skip  <graph->image.xplot_area ) {
                    thetime = localtime(&seconds);
                    snprintf(string, sizeof(string),"%s",month[thetime->tm_mon]);
                    gdImageString(*img, gdFontSmall,(coeff*(graph->image.xplot_area-skip))/xticks/2 + (i*(graph->image.xplot_area-skip))/xticks + skip +BORDER_L -7 ,
                        BORDER_T + graph->image.yplot_area + 5,string, std_colors[black]);
                    thetime->tm_mon++;
                    seconds=mktime(thetime);
                }
                gdImageSetStyle(*img, styleDottedDark, 3);
            } else {
                gdImageSetStyle(*img, styleDottedLight, 3);
            }
            gdImageLine(*img, (i*(graph->image.xplot_area-skip))/xticks + skip + BORDER_L, BORDER_T+1,
                    (i*(graph->image.xplot_area-skip))/(xticks) + skip +BORDER_L,
                    BORDER_T + graph->image.yplot_area -1, gdStyled);
        }
	}
	/* Draw Y-axis rate labels */
	for (i = 0; i <= graph->image.yplot_area; i += (graph->image.yplot_area / YTICKS)) {
		rate = (float)i *(1 / pixels_per_bit);
		plot_rate = rate / (graph->yunits);
		if (graph->ymax / graph->yunits < 10) {
			snprintf(string, sizeof(string), "%1.1f", plot_rate);
		} else if (graph->ymax / graph->yunits < 100) {
			snprintf(string, sizeof(string), "%2.0f", plot_rate);
		} else {
			snprintf(string, sizeof(string), "%3.0f", plot_rate);
		}
		if (rate > 0)
			gdImageString(*img, gdFontSmall, BORDER_L - 20, BORDER_T + graph->image.yplot_area - i - 5, string, std_colors[black]);
	}
}


void plot_labels(gdImagePtr *img, plot_obj_t *graph) {
	char string[BUFSIZE];
	float title_offset = 0.0;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Plotting labels (%s).\n", __FUNCTION__);

	if (graph->yunits == KILO)
		snprintf(string, sizeof(string), "K%s", graph->units);
	else if (graph->yunits == MEGA)
		snprintf(string, sizeof(string), "M%s", graph->units);
	else if (graph->yunits == GIGA)
		snprintf(string, sizeof(string), "G%s", graph->units);
	else
		snprintf(string, sizeof(string), "%s", graph->units);
	gdImageStringUp(*img, gdFontSmall, BORDER_L - 40, BORDER_T + (graph->image.yplot_area * 2 / 3), string, std_colors[black]);

	title_offset = (strlen(VERSION) + strlen(COPYRIGHT) + 2) * gdFontSmall->w;
	snprintf(string, sizeof(string), "%s %s", COPYRIGHT, VERSION);
	gdImageString(*img, gdFontSmall, BORDER_L + graph->image.xplot_area + BORDER_R - title_offset, BORDER_T - 15, string, std_colors[black]);
	title_offset = ( (graph->image.ximg_area)/2 - (strlen(graph->title)/2*gdFontSmall->w) );
	gdImageString(*img, gdFontSmall, title_offset, BORDER_T - 15, graph->title, std_colors[black]);
}


void create_graph(gdImagePtr * img, plot_obj_t *graph) {
	/* Create the image pointer */
	if (set->verbose >= HIGH) {
		fprintf(dfp, "\nCreating %d by %d image (%s).\n", graph->image.ximg_area,
		       graph->image.yimg_area, __FUNCTION__);
	}
	*img = gdImageCreate(graph->image.ximg_area, graph->image.yimg_area);
	gdImageInterlace(*img, TRUE);
}


void draw_arrow(gdImagePtr *img, plot_obj_t *graph) {
	int             red;
	gdPoint         points[3];
	int             xoffset, yoffset, size;

	if (set->verbose >= HIGH)
		fprintf(dfp, "Drawing directional arrow (%s).\n", __FUNCTION__);
	red = gdImageColorAllocate(*img, 255, 0, 0);
	xoffset = BORDER_L + graph->image.xplot_area - 2;
	yoffset = BORDER_T + graph->image.yplot_area;
	size = 4;
	points[0].x = xoffset;
	points[0].y = yoffset - size;
	points[1].x = xoffset;
	points[1].y = yoffset + size;
	points[2].x = xoffset + size;;
	points[2].y = yoffset;
	gdImageFilledPolygon(*img, points, 3, red);
}


void draw_border(gdImagePtr *img, plot_obj_t *graph) {
	int	xplot_area, yplot_area;

	xplot_area = graph->image.xplot_area;
	yplot_area = graph->image.yplot_area;

	/* Draw Plot Border */
	gdImageRectangle(*img, BORDER_L, BORDER_T, BORDER_L + xplot_area, BORDER_T + yplot_area, std_colors[black]);
	return;
}


void draw_grid(gdImagePtr *img, plot_obj_t *graph) {
	int             styleDotted[3];
	int             img_bg;
	int             plot_bg;
	int             i;
	int				ximg_area, yimg_area;
	int				xplot_area, yplot_area;

	img_bg = gdImageColorAllocate(*img, 235, 235, 235);
	plot_bg = gdImageColorAllocate(*img, 255, 255, 255);
	ximg_area = graph->image.ximg_area;
	yimg_area = graph->image.yimg_area;
	xplot_area = graph->image.xplot_area;
	yplot_area = graph->image.yplot_area;


	if (set->verbose >= HIGH)
		fprintf(dfp, "Drawing image grid (%s).\n", __FUNCTION__);

	gdImageFilledRectangle(*img, 0, 0, ximg_area, yimg_area, img_bg);
	gdImageFilledRectangle(*img, BORDER_L, BORDER_T, BORDER_L + xplot_area, BORDER_T + yplot_area, plot_bg);

	/* draw the image border */
	gdImageLine(*img, 0, 0, ximg_area - 1, 0, std_colors[light]);
	gdImageLine(*img, 1, 1, ximg_area - 2, 1, std_colors[light]);
	gdImageLine(*img, 0, 0, 0, yimg_area - 1, std_colors[light]);
	gdImageLine(*img, 1, 1, 1, yimg_area - 2, std_colors[light]);
	gdImageLine(*img, ximg_area - 1, 0, ximg_area - 1, yimg_area - 1, std_colors[black]);
	gdImageLine(*img, 0, yimg_area - 1, ximg_area - 1, yimg_area - 1, std_colors[black]);
	gdImageLine(*img, ximg_area - 2, 1, ximg_area - 2, yimg_area - 2, std_colors[black]);
	gdImageLine(*img, 1, yimg_area - 2, ximg_area - 2, yimg_area - 2, std_colors[black]);

	/* Define dotted style */
	styleDotted[0] = std_colors[light];
	styleDotted[1] = gdTransparent;
	styleDotted[2] = gdTransparent;
	gdImageSetStyle(*img, styleDotted, 3);

	/* Draw Image Grid Verticals */
      /*for (i = 1; i <= xplot_area; i++) {
		if (i % (xplot_area / XTICKS) == 0) {
			gdImageLine(*img, i + BORDER_L, BORDER_T, i + BORDER_L,
				    BORDER_T + yplot_area, gdStyled);
		}
      }*/
	/* Draw Image Grid Horizontals */
	for (i = 1; i <= yplot_area; i++) {
		if (i % (yplot_area / YTICKS) == 0) {
			gdImageLine(*img, BORDER_L, i + BORDER_T, BORDER_L + xplot_area,
				    i + BORDER_T, gdStyled);
		}
	}

	return;
}


void write_graph(gdImagePtr * img, char *output_file) {
	FILE           *pngout;

	if (output_file) {
		pngout = fopen(output_file, "wb");
		gdImagePng(*img, pngout);
		fclose(pngout);
	} else {
		fprintf(stdout, "Content-type: image/png\n\n");
		fflush(stdout);
		gdImagePng(*img, stdout);
	}
	gdImageDestroy(*img);
}


void init_colors(gdImagePtr * img, color_t ** colors) {
	color_t	*new = NULL;
	color_t	*last = NULL;
	int		i;
	int		red[NUM_LINE_COLORS] =  {0, 0, 255, 255, 255, 255, 138, 95, 173, 139};
	int		green[NUM_LINE_COLORS] ={235, 94, 0, 255, 185, 52, 43, 158, 255, 121};
	int		blue[NUM_LINE_COLORS] = {12, 255, 0, 0, 15, 179, 226, 160, 47, 94};

	if (set->verbose >= HIGH)
		fprintf(dfp, "Initializing colors (%s).\n", __FUNCTION__);
	/* Define some useful colors; first allocated is background color */
	std_colors[white] = gdImageColorAllocate(*img, 255, 255, 255);
	std_colors[black] = gdImageColorAllocate(*img, 0, 0, 0);
	std_colors[light] = gdImageColorAllocate(*img, 194, 194, 194);

	/* Allocate colors for the data lines */
	for (i = 0; i < NUM_LINE_COLORS; i++) {
		if ((new = (color_t *) malloc(sizeof(color_t))) == NULL) {
			fprintf(dfp, "Fatal malloc error in init_colors.\n");
		}
		new->rgb[0] = red[i];
		new->rgb[1] = green[i];
		new->rgb[2] = blue[i];
		new->shade = gdImageColorAllocate(*img, red[i], green[i], blue[i]);
		new->next = NULL;
		if (*colors != NULL) {
			last->next = new;
			last = new;
		} else {
			*colors = new;
			last = new;
		}
	}
	/* Make it a circular LL */
	new->next = *colors;
}


#ifdef HAVE_STRTOLL
long long intSpeed(int iid) {
#else
long intSpeed(int iid) {
#endif
    char            query[256];
#ifdef HAVE_STRTOLL
    long long speed;
#else
    long speed;
#endif

        debug(LOW, "Fetching interface speed (%s).\n", __FUNCTION__);
        
        snprintf(query, sizeof(query), "SELECT speed FROM interface WHERE id=%d", iid);
        debug(LOW, "Query String: %s\n", query);

        speed = db_intSpeed(query);
        debug(LOW, "Got speed of %ld\n",speed);
/* db_query(query); */
/*#if HAVE_MYSQL
	if (mysql_query(sql, query)) {
		fprintf(dfp, "Query error.\n");
		return (-1);
	}
	if ((result = mysql_store_result(sql)) == NULL) {
		fprintf(dfp, "Retrieval error.\n");
		return (-1);
	}
	row = mysql_fetch_row(result);
  #ifdef HAVE_STRTOLL
	return strtoll(row[0], NULL, 0);
  #else
	return strtol(row[0], NULL, 0);
  #endif
#elif HAVE_PGSQL
	result = pgsql_db_query(query, sql);
  #ifdef HAVE_STRTOLL
	return strtoll(result, NULL, 0);
  #else
	return strtol(result, NULL, 0);
  #endif
#endif
*/
    return speed;
}


/* given area of plot, size image with space for borders, legend, etc */
int sizeImage(plot_obj_t *PO, int LOs) {
	PO->image.border_b = BORDER_B + 10*LOs;
	PO->image.ximg_area = (unsigned int)(PO->image.xplot_area + BORDER_L + BORDER_R);
	PO->image.yimg_area = (unsigned int)(PO->image.yplot_area + BORDER_T + PO->image.border_b);
	return (1);
}


/* Initialize plot sizes */
void sizeDefaults(plot_obj_t *PO) {
	PO->image.xplot_area = XPLOT_AREA;
	PO->image.yplot_area = YPLOT_AREA;
	PO->image.border_b = BORDER_B;
	PO->image.ximg_area = XIMG_AREA;
	PO->image.yimg_area = YIMG_AREA;
}


/* Compare two data_t's */
float cmp(data_t *a, data_t *b) {
    return b->rate - a->rate;
}


/* Sort a data_t linked list */
data_t *sort_data(data_t *list, int is_circular, int is_double) {
    data_t *p, *q, *e, *tail, *oldhead;
    int insize, nmerges, psize, qsize, i;

    /* if `list' was passed in as NULL, return immediately */
    if (!list)
       return NULL;

    insize = 1;

    while (1) {
        p = list;
        oldhead = list; /* only used for circular linkage */
        list = NULL;
        tail = NULL;

        nmerges = 0;  /* count number of merges we do in this pass */

        while (p) {
            nmerges++;  /* there exists a merge to be done */
            /* step `insize' places along from p */
            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) {
                psize++;
               if (is_circular)
                   q = (q->next == oldhead ? NULL : q->next);
               else
                   q = q->next;
                if (!q) break;
            }

            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;

            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) {

                /* decide whether next element of merge comes from p or q */
                if (psize == 0) {
                   /* p is empty; e must come from q. */
                   e = q; q = q->next; qsize--;
                   if (is_circular && q == oldhead) q = NULL;
               } else if (qsize == 0 || !q) {
                   /* q is empty; e must come from p. */
                   e = p; p = p->next; psize--;
                   if (is_circular && p == oldhead) p = NULL;
               } else if (cmp(p,q) <= 0) {
                   /* First element of p is lower (or same);
                    * e must come from p. */
                   e = p; p = p->next; psize--;
                   if (is_circular && p == oldhead) p = NULL;
               } else {
                   /* First element of q is lower; e must come from q. */
                   e = q; q = q->next; qsize--;
                   if (is_circular && q == oldhead) q = NULL;
               }

                /* add the next element to the merged list */
               if (tail) {
                   tail->next = e;
               } else {
                   list = e;
               }
               if (is_double) {
                   /* Maintain reverse pointers in a doubly linked list. */
                   /* e->prev = tail; */
               }
               tail = e;
            }

            /* now p has stepped `insize' places along, and q has too */
            p = q;
        }
       if (is_circular) {
           tail->next = list;
			/*
           if (is_double)
             list->prev = tail;
			*/
       } else
           tail->next = NULL;

        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
            return list;

        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
}


/* Count elements in a data_t list.  We can probably get this free
   elsewhere, but use this for now */
unsigned int count_data(data_t *head) {
	int count = 0;
	data_t *ptr = NULL;

	ptr = head;
	while (ptr) {
		count++;
		ptr = ptr->next;
	}
	return count;
}


/* Return the Nth largest element in a sorted data_t list, i.e. 95th % */
data_t *return_Nth(data_t *head, int datapoints, int n) {
	int depth;
	float percent = 0.0;
	int i;

	percent = 1 - ((float) n/100);
	depth = datapoints * percent;
		
	for (i=0;i<depth;i++) {
		head = head->next;
	}
	return(head);
}


void parseCmdLine(int argc, char **argv, arguments_t * arguments, plot_obj_t * graph) {
	int	ch;
	data_obj_t *DO = NULL;
	line_obj_t *LO = NULL;
	char *temp = NULL;

	arguments->output_file = NULL;

	while ((ch = getopt(argc, argv, "c:d:hl:o:p:v")) != EOF)
		switch ((char)ch) {
		case 'c':
			arguments->conf_file = optarg;
			break;
        case 'd':
            count.DOs++;
            DO = parseDO(optarg);
            addDO(DO->index, DO);
            if (DO->index > count.DOs) count.DOs = DO->index;
            break;
        case 'l':
            count.LOs++;
            LO = parseLO(optarg);
            addLO(LO->index, LO);
            if (LO->index > count.LOs) count.LOs = LO->index;
            break;
		case 'o':
			arguments->output_file = optarg;
			break;
        case 'p':
            count.POs++;
            parsePO(PO, optarg);
            break;
		case 'v':
			set->verbose++;
			break;
		case 'h':
		default:
			usage(argv[0]);
			break;
		}

	checkCount(&count);
        set->daemon = 0;
	if (set->verbose >= LOW) 
		fprintf(dfp, "RTGplot version %s.\n", VERSION);
}


void parseWeb(arguments_t *arguments, plot_obj_t *graph) {
	s_cgi	**cgiArg;
	data_obj_t *DO = NULL;
	line_obj_t *LO = NULL;
	char	*temp = NULL;
	char	*token = NULL;
	char	var[BUFSIZE];

	cgiDebug(0, 0);
	cgiArg = cgiInit();

	count.POs = cgiGetValueCount(cgiArg, "PO");
	count.DOs = cgiGetValueCount(cgiArg, "DO");
	count.LOs = cgiGetValueCount(cgiArg, "LO");
	
	checkCount(&count);
	
    /* REB -- These are undocumented, still needed? */
	if (cgiGetValue(cgiArg, "html")) {
		HTMLOUT = 1;
	}
	if ((temp = cgiGetValue(cgiArg, "interval"))) {
		set->interval = atoi(temp);
	}

	if ((temp = cgiGetValue(cgiArg, "debug"))) {
		snprintf(var, sizeof(var), "%s.%s", DEBUGLOG, file_timestamp());
		dfp = fopen(var, "w"); 
		set->verbose = atoi(temp);
	}

	if ((temp = cgiGetValue(cgiArg, "PO"))) {
		parsePO(PO, temp);
	}
	if ((temp = cgiGetValue(cgiArg, "DO"))) {
		token = strtok(temp, "\n");
		while (token) {
			DO = parseDO(token);
			addDO(DO->index, DO);
			if (DO->index > count.DOs) count.DOs = DO->index;
			token = strtok(NULL, "\n");
		}
	}
	if ((temp = cgiGetValue(cgiArg, "LO"))) {
		token = strtok(temp, "\n");
		while (token) {
			LO = parseLO(token);
			addLO(LO->index, LO);
			if (LO->index > count.LOs) count.LOs = LO->index;
			token = strtok(NULL, "\n");
		}
	}
	if (HTMLOUT) {
		dfp = stdout;
		set->verbose = HIGH;
	}
}


void checkCount(count_t *c) {
    if (c->POs == 0) plotbail("WARNING: No Plot Objects Defined!");
    if (c->POs > 1)  plotbail("WARNING: Multiple Plot Objects Defined!");
    if (c->LOs == 0) plotbail("WARNING: No Line Objects Defined!");
    if (c->DOs == 0) plotbail("WARNING: No Data Objects Defined!");

	if (HTMLOUT) {
		cgi_header();
    	printf("<P><I>Object Counts</I>:\n");
    	printf("<BLOCKQUOTE>\n");
    	printf("POs: %d<BR>\n", c->POs);
    	printf("DOs: %d<BR>\n", c->DOs);
    	printf("LOs: %d<BR>\n", c->LOs);
    	printf("</BLOCKQUOTE>\n");
	}

}   


/* Add pointer into position pos of variable sized pointer array */
void addDO(int pos, void *insert) {
    static unsigned int size = 0;
    static unsigned int capacity = 0;
	float exp;
	int oldsize=0;
	int i;

	plotdebug("Adding DO element: %d\n", pos);
    // create initial chunk of memory
    if (size == 0) {
        capacity = size = 5;
        DOs = (data_obj_t **) malloc(size * sizeof(void *));
        if (DOs == NULL) plotbail("malloc failure!");
		for(i=0;i<size;i++) 
			DOs[i] = NULL;
    }
    // no more room, double existing size
    if (capacity == 0) {
        capacity = size;
        size*=2;
        plotdebug("realloc to %d elements", size);
        DOs = (data_obj_t **) realloc(DOs, size * sizeof(void *));
        if (DOs == NULL) plotbail("realloc failure!");
		for(i=capacity;i<size;i++) 
			DOs[i] = NULL;
    }
	if (pos >= size) {
		oldsize = size;
		exp = ceil( (log((float)pos/size)/log(2)) );
		capacity+= size * ((int) pow(2,exp) - 1);
		if (pos == size) exp = 1;
		size*= (int) pow(2,exp);
        plotdebug("realloc to %d elements, free: %d", size, capacity);
        DOs = (data_obj_t **) realloc(DOs, size * sizeof(void *));
        if (DOs == NULL) plotbail ("realloc failure!");
		for(i=oldsize;i<size;i++) 
			DOs[i] = NULL;
	}
    DOs[pos] = insert;
    capacity--;
}


/* Add pointer into position pos of variable sized pointer array */
void addLO(int pos, void *insert) {
    static unsigned int size = 0;
    static unsigned int capacity = 0;
	int exp;
	int oldsize=0;
	int i;

	plotdebug("Adding LO element: %d\n", pos);
    // create initial chunk of memory
    if (size == 0) {
        capacity = size = 5;
        LOs = (line_obj_t **) malloc(size * sizeof(void *));
        if (LOs == NULL) plotbail ("malloc failure!");
		for(i=0;i<size;i++) 
			LOs[i] = NULL;
    }
    // no more room, double existing size
    if (capacity == 0) {
        capacity = size;
        size*=2;
        plotdebug("realloc to %d elements", size);
        LOs = (line_obj_t **) realloc(LOs, size * sizeof(void *));
        if (LOs == NULL) plotbail ("realloc failure!");
		for(i=capacity;i<size;i++) 
			LOs[i] = NULL;
    }
	if (pos >= size) {
		oldsize = size;
		exp = ceil( (log((float)pos/size)/log(2)) );
		capacity+= size * ((int) pow(2,exp) - 1);
		if (pos == size) exp = 1;
		size*= (int) pow(2,exp);
        plotdebug("realloc to %d elements, free: %d", size, capacity);
        LOs = (line_obj_t **) realloc(LOs, size * sizeof(void *));
        if (LOs == NULL) plotbail ("realloc failure!");
		for(i=oldsize;i<size;i++) 
			LOs[i] = NULL;
	}
    LOs[pos] = insert;
    capacity--;
}


void printPOs() {
		printf("<P><I>Plot Object(PO)</I>: <BR>\n");
		printf("<BLOCKQUOTE>\n");
		if (PO) {
			printf("Title: %s XSize: %u YSize: %u ScaleX: %u ScaleY: %u <BR>\n",
				PO->title, PO->image.ximg_area, PO->image.yimg_area, PO->scalex, PO->scaley);
            printf("Units: %s Impulse: %d Gauge: %d <BR>\n",
                PO->units, PO->impulse, PO->gauge);
			printf("XPlot: %u YPlot: %u BorderB: %u <BR>\n",
				PO->image.xplot_area, PO->image.yplot_area, PO->image.border_b);
			printf("Xmax: %d Ymax: %f Xoff: %lu Xunit: %d Yunit: %lld <BR>\n",
				PO->xmax, PO->ymax, PO->xoffset, PO->xunits, PO->yunits);
			printf("Begin: %lu End: %lu <BR>\n",
				PO->range.begin, PO->range.end);
		}
		printf("</BLOCKQUOTE>\n");
}


void printDOs() {
	int i;

	printf("<P><I>Data Objects(DO)</I>: <BR>\n");
	printf("<BLOCKQUOTE>\n");
	for (i=0;i<=count.DOs;i++) {
		if (DOs[i] != NULL) {
			printf("DO[%u/%d]: Table %s Id: %u Points: %lu<BR>\n", 
				DOs[i]->index, i, DOs[i]->table, DOs[i]->id, DOs[i]->datapoints);
		}
	}
	printf("</BLOCKQUOTE>\n");
}


void printLOs() {
	data_obj_list_t *DOlist = NULL;
	int i;

	printf("<P><I>Line Objects(LO)</I>: <BR>\n");	
	printf("<BLOCKQUOTE>\n");
	for (i=0;i<=count.LOs;i++) {
		if (LOs[i] != NULL) {
			printf("LO[%u/%d]: Aggr: %u Percent: %u Filled: %u Factor: %u Label: %s<BR>\n", 
				LOs[i]->index, i, LOs[i]->aggr, LOs[i]->percentile, LOs[i]->filled,
				LOs[i]->factor, LOs[i]->label);
			printf("LO[%u] contains DOs: <BR>\n", LOs[i]->index);
			printf("<BLOCKQUOTE>\n");
			DOlist = LOs[i]->DO_list;				
			while (DOlist) {
				if (DOlist->DO != NULL) {
					printf("DO[%d]: Table: %s IID: %u <BR>\n", 
						DOlist->DO->index, DOlist->DO->table, DOlist->DO->id);
				} else {
					plotbail("Line Object problem!");
				} 
				DOlist = DOlist->next;
			}
			if (LOs[i]->DO_aggr != NULL) {
				printf("DOAggr[]: IID: %u <BR>\n",
					LOs[i]->DO_aggr->id);
			}
			printf("</BLOCKQUOTE>\n");
		}
	}
	printf("</BLOCKQUOTE>\n");
}


/* clear Empty DOs and LOs.  ugly - wanting for STL right now... */
void clearEmpties() {
	data_obj_list_t		*head = NULL;
	data_obj_list_t		**last = NULL;
	data_obj_list_t		**DOLptr = NULL;
	int i;

	// remove empty DOs from LOs
	for (i=0;i<=count.LOs;i++) {
		if (LOs[i]) {
			head = LOs[i]->DO_list;
			DOLptr = &(LOs[i]->DO_list);
			last = DOLptr;
			while (*DOLptr != NULL) {
				if ((*DOLptr)->DO->datapoints <= 0) {
					(LOs[i]->DO_count)--;
					// delete from head
					if ( (*DOLptr == head) && ((*DOLptr)->next != NULL) ) {
						head = head->next;
						(*DOLptr) = head;
						last = DOLptr;
					}
					// delete from head, no more DOs
					else if ( (*DOLptr == head) && ((*DOLptr)->next == NULL) ) {
						*DOLptr = NULL;
					}
					// delete from end
					else if ( (*DOLptr)->next == NULL ) {
						*DOLptr = NULL;
					}
					// delete from middle
					else  {
						(*last)->next = (*DOLptr)->next;
						DOLptr = &((*DOLptr)->next);
					}
				} else {
					last = DOLptr;
					DOLptr = &((*DOLptr)->next);
				}
			}
		}
	}
	// remove empty LOs
	for (i=0;i<=count.LOs;i++) {
		if ( (LOs[i] != NULL) && (LOs[i]->DO_count <= 0) ) {
			LOs[i] = NULL;
		}
	}
	// remove empty DOs
	for (i=0;i<=count.DOs;i++) {
		if ( (DOs[i] != NULL) && (DOs[i]->datapoints <= 0) ) {
			DOs[i] = NULL;
		}
	}
}


/* XXX - REB - can probably be blown away with small mod to calculate_rate */
void rateFactor(rate_t *rate, int factor) {
	(rate->total)*=factor;
	(rate->max)*=factor;
	(rate->avg)*=factor;
	(rate->cur)*=factor;
}

