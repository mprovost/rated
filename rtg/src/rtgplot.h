/****************************************************************************
   Program:     $Id: rtgplot.h,v 1.32 2008/01/19 03:01:32 btoneill Exp $ 
   Author:      $Author: btoneill $
   Date:        $Date: 2008/01/19 03:01:32 $
   Orig Date:   January 15, 2002
   Description: RTG traffic grapher headers
****************************************************************************/

#ifndef _RTGPLOT_H_
#define _RTGPLOT_H_ 1

#include <gd.h>
#include <gdfonts.h>

/* Image Defaults */
#define XPLOT_AREA 500
#define YPLOT_AREA 150
#define BORDER_T 20
#define BORDER_B 50
#define BORDER_L 50
#define BORDER_R 20
#define LEGENDOFFSET -20
#define XIMG_AREA (unsigned int)(XPLOT_AREA + BORDER_L + BORDER_R)
#define YIMG_AREA (unsigned int)(YPLOT_AREA + BORDER_T + BORDER_B)
#define XTICKS 10
#define YTICKS 5

/* Maximums */
/* XXX - REB - what's the right bounds checking here? */
#define XPLOT_AREA_MAX 3000
#define YPLOT_AREA_MAX 3000

#define MINUTE 60
#define HOUR (unsigned int)(MINUTE * 60)
#define DAY (unsigned int)(HOUR * 24)
#define WEEK (unsigned int)(DAY * 7)
#define MONTH (unsigned int)(WEEK * 4)

#define DEFAULT_UNITS "bps"
#define NUM_LINE_COLORS 10
#define LEGEND_MAX_LEN 17
#define COPYRIGHT "RTG"
#define DEBUGLOG "/tmp/rtgplot.log"


/* Populate a data_t per item to plot */
typedef struct data_struct {
    long long counter;		// interval sample value
    unsigned long timestamp;	// UNIX timestamp
    float rate;			// floating point rate
    int x;			// X plot coordinate
    int y;			// Y plot coordinate
    struct data_struct *next;	// next sample
} data_t;

/* If calculating rate, a rate_t stores total, max, avg, cur rates */
typedef struct rate_struct {
    unsigned long long total;
    float max;
    float avg;
    float cur;
} rate_t;

/* Each graph uses a range_t to keep track of data ranges */
typedef struct range_struct {
    unsigned long begin;	// UNIX begin time
    unsigned long end;		// UNIX end time
    unsigned long dataBegin;	// Actual first datapoint in database
    long long counter_max;	// Largest counter in range
    int scalex;			// Scale X values to match actual datapoints
    long datapoints;		// Number of datapoints in range
} range_t;

/* XXX - REB - new structs */
typedef struct count_struct {
    unsigned int POs; 
    unsigned int DOs;
    unsigned int LOs;
} count_t;

typedef struct data_obj {
    unsigned int index;
    char *table;
    unsigned int id;
	unsigned long datapoints;
    unsigned long dataBegin;	// Actual first TS datapoint 
    unsigned long dataEnd;		// Actual last TS datapoint 
    long long counter_max;		// Largest counter value
	data_t *data; // ptr to head of data linked list -- needs to be populated()
	rate_t rate_stats;
} data_obj_t;

/* We may wish to construct arbitrary groupings of data objects (DO).
   This is one particular wrapper: a linked-list of DOs */
typedef struct data_obj_list {
    data_obj_t *DO;
    struct data_obj_list *next;
} data_obj_list_t;

/* Each graph has a image_t struct to keep borders and area variables */
typedef struct image_struct {
    unsigned int xplot_area;	// X pixels of the plot
    unsigned int yplot_area;	// Y pixels of the plot
    unsigned int border_b;	// Pixels of border from plot to image bottom
    unsigned int ximg_area;	// X pixels of the entire image
    unsigned int yimg_area;	// Y pixels of the entire image
} image_t;

typedef struct plot_obj {
    char *title;
	int xmax;				// largest span of seconds on x-axis
	float ymax;				// largest y-axis value
	unsigned long xoffset;	// all time is relative to xoffset
	int xunits;				// HOUR/DAY/WEEK/MONTH
	long long yunits;		// 1/K/M/G/T
    char *units;			// units as a string
    unsigned short impulse;
    unsigned short gauge;
    unsigned short scalex;
    unsigned short scaley;
	image_t image;
    range_t range;
} plot_obj_t;

/* index:data:modifiers:appearance:label */
typedef struct line_obj {
    unsigned int index;
    data_obj_list_t *DO_list;
	data_obj_t *DO_aggr;		// aggregated DO, created by aggr()
	unsigned int DO_count;		// number of DOs in DO_list
    unsigned short aggr;
    unsigned short percentile;
    unsigned short filled;
    unsigned short factor;
	int shade;
    char *label;
} line_obj_t;
/* XXX - REB - new structs */

/* A linked list of colors that we iterate through each line */
typedef struct color_struct {
    int shade;
    int rgb[3];
    struct color_struct *next;
} color_t;

typedef struct arguments_struct {
    char *conf_file;
    char *output_file;
} arguments_t;

/* Globals */
enum major_colors {white, black, light};
int std_colors[3];


/* Precasts: rtgplot.c */
void dump_data(data_t *);
int populate(char *, data_obj_t *);
void normalize(data_t *, plot_obj_t *, int);
void usage(char *);
void dump_rate_stats(rate_t *);
void plot_line(gdImagePtr *,  plot_obj_t *, line_obj_t *);
void plot_Nth(gdImagePtr *, plot_obj_t *, data_t *, int);
void create_graph(gdImagePtr *, plot_obj_t *);
void draw_grid(gdImagePtr *, plot_obj_t *);
void draw_border(gdImagePtr *, plot_obj_t *);
void draw_arrow(gdImagePtr *, plot_obj_t *);
void write_graph(gdImagePtr *, char *);
void plot_scale(gdImagePtr *, plot_obj_t *);
void plot_labels(gdImagePtr *, plot_obj_t *);
void plot_legend(gdImagePtr *, plot_obj_t *, line_obj_t *, int);
void init_colors(gdImagePtr *, color_t **);
void calculate_rate(data_t **, rate_t *);
void calculate_total(data_t **, rate_t *);
#ifdef HAVE_STRTOLL
long long intSpeed(int);
#else
long intSpeed(int);
#endif
void sizeDefaults(plot_obj_t *);
int sizeImage(plot_obj_t *, int);
float cmp(data_t *, data_t *);
data_t *sort_data(data_t *data, int is_circular, int is_double );
unsigned int count_data(data_t *);
data_t *return_Nth(data_t *, int, int);
void parseCmdLine(int, char **, arguments_t *, plot_obj_t *);
void parseWeb(arguments_t *, plot_obj_t *);
void dataAggr(data_t **, data_t *, rate_t *, rate_t *);
char *file_timestamp();
/* REB - XXX NEW */
void cgi_header();
void plotbail(char *s);
void checkCount(count_t *c);
void addDO(int pos, void *insert);
void addLO(int pos, void *insert);
void printPOs();
void printDOs();
void printLOs();
int copyDO(data_obj_t *, data_obj_t *);
void clearEmpties();
void rateFactor(rate_t *, int);

/* Precasts: rtgparse.c */
char *getField(char *c, char sep, char **result);
void parse(char *string);
data_obj_t *parseDO(char *string);
void parseLOmod(char *string, line_obj_t **LOptr);
void parsePOmod(char *string, plot_obj_t **POptr);
void parseLODO(char *string, line_obj_t **LOptr);
line_obj_t *parseLO(char *string);
plot_obj_t *parsePO(plot_obj_t *PO, char *string);

#endif /* not _RTGPLOT_H_ */
