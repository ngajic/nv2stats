/* ============================================================================================== */
/*                                   Include C Library Files                                      */
/* ============================================================================================== */
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================================== */
/*                                         Include Files                                          */
/* ============================================================================================== */
#include "functions.h"
#include "util.h"
#include "base64.h"
#include "sqlite3.h"
#include "cypher.h"
#include "zlib.h"

/* ============================================================================================== */
/*                                             Private                                            */
/* ============================================================================================== */

#define NUM_LEVELS  600         // Number of metanet levels
#define NUM_ENTRIES 10          // Number of spots on leaderboard
#define MAX_NAME_LEN    31      // Maximal length of the player's nickname

#define TYPE_INT        1       // Used for functions which work with generic datatypes
#define TYPE_FLOAT      2       // Used for functions which work with generic datatypes

#define SQLITE_INT_RANGE_CHECK(val) do {if ((val) > INT_MAX || (val) < INT_MIN) return SQLITE_RANGE;} while(0)
#define SQLITE_USHRT_INT_RANGE_CHECK(val) do {if ((val) > USHRT_MAX || (val) < 0) return SQLITE_RANGE;} while(0)

#define IGNORE_RESULT(x) do { int _r = (x); (void)_r; } while(0)

//! General placeholder for callback functions which keeps information about player on a given level
struct storage {
    char day[11];                       // Day string (eg. "2013-07-28"
    char nick[MAX_NAME_LEN + 1];        // Player nickname string
    int score;                          // Player's score
};

//! Structure which stores statistical information about some player
struct stats {
    char nick[MAX_NAME_LEN + 1];        // Player nickname string
    int pure;                           // Number of pure 0ths
    int ties;                           // Number of tied 0ths
    int top5s;                          // Number of top5 scores
    int top10s;                         // Number of top10 scores
    int pts;                            // Number of points
};

struct sql_callback_data {
    char format[32];
    //! Structure which stores statistical information about some player
    size_t stats_cnt;
    struct stats *stats;
};

//! Date structure
struct date {
    int yy;         // year
    int mm;         // month
    int dd;         // day
};

//! Hard template for footer of index page
static const char *index_footer = { "\t</ul>\n\t</marquee>\n\t</aside>\n\n\t</main>\n\n\t<footer>\n\t\t<p>Have spare time? Use your CSS and HTML skills at this "
                        "<a href=\"https://github.com/ngajic/ngajic.github.io\">repository</a> where this site is hosted.</p>\n\t</footer>\n\t</body>\n</html>\n" };

//! Hard template for level history page, includes Javascript scripts for example
static const char *header = { "<!DOCTYPE HTML>\n<html lang=\"en-US\">\n\t<head>\n\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\t\t"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n\t\t<title>Levels 0ths History</title>\n\n\n\t\t"
                       "<link rel=\"stylesheet\" type=\"text/css\" href=\"../css/styles.css\">\n\t\t<script>\n\t\t\tfunction showGraph(level) {"
                       "\n\t\t\t\tvar graph = document.getElementById(\"container\" + level);\n\n\t\t\t\tif (graph.style.display == \"none\") {"
                       "\n\t\t\t\t\tgraph.style.display = \"block\";\n\t\t\t\t}\n\t\t\t\telse {\n\t\t\t\t\tgraph.style.display = \"none\";\n\t\t\t\t}"
                       "\n\t\t\t}\n\t\t\tfunction collapseAll() {\n\t\t\t\tfor (var i = 0; i < 600; i++) {\n\t\t\t\t\tdocument.getElementById(\"container\" + i)"
                       ".style.display = \"none\";\n\t\t\t\t}\n\n\t\t\t\tvar cbarray = document.getElementsByName(\"box\");\n\t\t\t\tfor(var i = 0; i < cbarray.length; i++){"
                       "\n\t\t\t\t\tif (cbarray[i].checked == true) {\n\t\t\t\t\t\tcbarray[i].checked = false;\n\t\t\t\t\t}\n\t\t\t\t}\n\t\t\t}\n\t\t</script>\n\t</head>\n\t<body>\n\t"
                       "<script src=\"../code/highcharts.js\"></script>\n\t<script src=\"../code/js/themes/grid.js\"></script>\n\t<!--script src=\"../code/modules/series-label.js\"></script-->\n\t"
                       "\n\n\t<header>\n\t\t<h1>Welcome to</h1>\n\t\t<h1>Nv2 Levels charts</h1>\n\t</header>\n\n\t<main>\n\n\t\t<nav>\n\t\t<ul>\n\t\t\t"
                       "<li><a class=\"aside-link\" href=\"../index.htm\">Home</a></li>\n\t\t\t<li><a class=\"aside-link\" href=\"current-rankings-charts.htm\">Latest rankings</a></li>\n\t\t\t"
                       "<li><a class=\"aside-link\" href=\"0thRankingsChart.htm\">Timeline charts</a></li>\n\t\t\t<li><a class=\"aside-link\" href=\"#\">Level History</a></li>\n\t\t\t"
                       "<li><a class=\"aside-link\" href=\"heatmaps/shomman-heatmap.htm\">Position maps</a></li>\n\t\t\t<li><a class=\"aside-link\" href=\"../about.htm\">About</a></li>\n\t\t"
                       "</ul>\n\t\t</nav>\n\n\t\t<form action=\"\" onsubmit=\"return false;\">\n" };

//! Template for each level div container in level history page
static const char *highchart = { "\t<div id=\"container%u\" style=\"display: %s; min-width: 310px; height: 600px; margin: 0 auto\">"
                                 "</div>\n\n\t\t<script type=\"text/javascript\">\n\nHighcharts.chart('container%u', {\n\tchart: {\n\t\ttype: 'spline',\n\t\tzoomType: 'xy'\n\t},\n\ttitle: {\n\t\ttext: '%u-%u Level Owner history'\n\t"
                                 "},\n\tsubtitle: {\n\t\ttext: 'Irregular time data in Highcharts JS'\n\t},\n\txAxis: {\n\t\ttype: 'datetime',\n\t\ttitle: {\n\t\t\ttext: 'Date'\n"
                                 "\t\t}\n\t},\n\tyAxis: {\n\t\ttitle: {\n\t\t\ttext: 'Score'\n\t\t},\n\t\tmin: %d\n\t},\n\ttooltip: {\n\t\theaderFormat: '<b>{series.name}</b><br>',\n"
                                 "\t\tpointFormat: '{point.x:%%e. %%b}: {point.y:%%f} frames'\n\t},\n\tplotOptions: {\n\t\tspline: {\n\t\t\tmarker: {\n\t\t\t\tenabled: true\n\t\t\t"
                                 "}\n\t\t},\n\t\tseries: {\n\t\t\tcursor: 'pointer'\n\t\t}\n\t},\n\n\tseries: [{\n" };

//! Template for current rankings chart
static const char *curr_header = { "<!DOCTYPE HTML>\n<html lang=\"en-US\">\n\t<head>\n\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\t\t"
                            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n\t\t<title>Current rankings</title>\n\n\n\t\t"
                            "<link rel=\"stylesheet\" type=\"text/css\" href=\"../css/styles.css\">\n\t</head>\n\t<body>\n\t<script src=\"../code/highcharts.js\">"
                            "</script>\n\t<script src=\"../code/highcharts-more.js\"></script>\n\t<script src=\"../code/modules/item-series.js\"></script>\n\n\n\t"
                            "<header id=\"header-timeline\">\n\t\t<h1>Welcome to</h1>\n\t\t<h1>Nv2 latest charts</h1>\n\n\t\t<nav>\n\t\t<ul>\n\t\t\t<li><a href=\"../index.htm\">"
                            "Home</a></li\n\t\t\t><li><a href=\"../about.htm\">About</a></li>\n\t\t</ul>\n\t\t</nav>\n\t</header>\n\n" };

//! Hard template for heatmap pages header and starting body
static const char *player_heatmap = { "<!DOCTYPE HTML>\n<html lang=\"en-US\">\n\t<head>\n\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n\t\t"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n\t\t<title>Heatmap Demo</title>\n\n\n\n\t\t<link rel=\"stylesheet\" type=\"text/css\" href=\"../../css/styles.css\">"
                       "\n\t</head>\n\t<body>\n\t<script src=\"../../code/highcharts.js\"></script>\n\t<script src=\"../../code/modules/heatmap.js\"></script>\n\t<script src=\"../../code/modules/exporting.js\"></script>\n\t"
                       "<script src=\"../../code/modules/export-data.js\"></script>\n\n\t<header id=\"header-timeline\">\n\t\t<h1>Welcome to</h1>\n\t\t<h1>Nv2 heatmaps</h1>\n\n\t\t<nav>\n\t\t<ul>\n\t\t\t<li><a href="
                       "\"../../index.htm\">Home</a></li\n\t\t\t><li><a href=\"shomman-heatmap.htm\">shomman</a></li\n\t\t\t><li><a href=\"eddymataga-heatmap.htm\">EddyMataGallos</a></li\n\t\t\t"
                       "><li><a href=\"oxymoron93-heatmap.htm\">oxymoron93</a></li\n\t\t\t><li><a href=\"jp27ace-heatmap.htm\">jp27ace</a></li\n\t\t\t><li><a href=\"kkstrong-heatmap.htm\">kkstrong</a></li\n\t\t\t"
                       "><li><a href=\"cooperverd-heatmap.htm\">cooperverdon</a></li\n\t\t\t><li><a href=\"swipenet-heatmap.htm\">swipenet</a></li\n\t\t\t><li><a href=\"natakial2-heatmap.htm\">natakial2</a></li\n\t\t\t"
                       "><li><a href=\"polt-heatmap.htm\">Polt</a></li\n\t\t\t><li><a href=\"v3l0c1t4-heatmap.htm\">V3L0C1T4</a></li\n\t\t\t><li><a href=\"danielr-heatmap.htm\">danielr</a></li\n\t\t\t"
                       "><li><a href=\"gnarcissis-heatmap.htm\">Gnarcissist</a></li\n\t\t\t><li><a href=\"icy-sole-heatmap.htm\">Icy-Sole</a></li\n\t\t\t><li><a href=\"oleary15-heatmap.htm\">Oleary15</a></li\n\t\t\t"
                       "><li><a href=\"patfelix-heatmap.htm\">patfelix</a></li\n\t\t\t><li><a href=\"thisguy248-heatmap.htm\">Thisguy248</a></li\n\t\t\t><li><a href=\"btoasterbo-heatmap.htm\">BtoasterBoy</a></li\n\t\t\t"
                       "><li><a href=\"tailow-heatmap.htm\">tailow</a></li\n\t\t\t><li><a href=\"pzyll-heatmap.htm\">pzylL</a></li\n\t\t\t><li><a href=\"golfkid-heatmap.htm\">golfkid</a></li\n\t\t\t"
                       "><li><a href=\"../../about.htm\">About</a></li>\n\t\t</ul>\n\t\t</nav>\n\t</header>\n\n\t<div id=\"container\" style=\"min-width: 310px; height: 600px; margin: 0 auto\"></div>"
                       "\n\n\t\t<script type=\"text/javascript\">\n\n" };

//! Template for rest of the heatmap page for particular player
static const char *heatmap = { "Highcharts.chart('container', {\n\n\tchart: {\n\t\ttype: 'heatmap',\n\t\tmarginTop: 40,\n\t\tmarginBottom: 80,\n\t\tplotBorderWidth: 1\n\t},\n\n\n\ttitle: {\n\t\ttext: '%s positions'\n\t},"
                        "\n\n\txAxis: {\n\t\tcategories: ['00', '01', '02', '03', '04', '05', '06', '07', '08', '09', '10', '11', '12', '13', '14', '15', '16', '17', '18', '19', '20', '21', '22', '23', '24', '25',"
                        " '26', '27', '28', '29', '30', '31', '32', '33', '34', '35', '36', '37', '38', '39', '40', '41', '42', '43', '44', '45', '46', '47', '48', '49', '50', '51', '52', '53', '54', '55', '56', '57', '58', '59',"
                        " '60', '61', '62', '63', '64', '65', '66', '67', '68', '69', '70', '71', '72', '73', '74', '75', '76', '77', '78', '79', '80', '81', '82', '83', '84', '85', '86', '87', '88', '89', '90',"
                        " '91', '92', '93', '94', '95', '96', '97', '98', '99', '100', '101', '102', '103', '104', '105', '106', '107', '108', '109', '110', '111', '112', '113', '114', '115', '116', '117', '118', '119']\n\t},\n\n\t"
                        "yAxis: {\n\t\tcategories: ['0', '1', '2', '3', '4'],\n\t\ttitle: null\n\t},\n\n\tcolorAxis: {\n\t\tmin: 0,\n\t\tdataClassColor: 'category',\n\t\tdataClasses: [{\n\t\t\tcolor: '#FF0000',"
                        "\n\t\t\tfrom: 0,\n\t\t\tname: '0th',\n\t\t\tto: 0\n\t\t}, {\n\t\t\tcolor: '#FF6666',\n\t\t\tfrom: 1,\n\t\t\tname: '1-4',\n\t\t\tto: 4\n\t\t}, {\n\t\t\tcolor: '#FFDDDD',\n\t\t\tfrom: 5,\n\t\t\tname: '5-9',"
                        "\n\t\t\tto: 9\n\t\t}, {\n\t\t\tcolor: '#000000',\n\t\t\tfrom: 10,\n\t\t\tname: 'No score',\n\t\t\tto: 10\n\t\t}]\n\t},\n\n\ttooltip: {\n\t\tformatter: function () {\n\t\t\treturn '<b>'"
                        " + this.series.xAxis.categories[this.point.x] + '</b>-<b>' + this.series.yAxis.categories[this.point.y] + '</b><br><b>pos: ' + (this.point.value === 10 ? 'X' : this.point.value) + '</b>';\n\t\t}\n\t},"
                        "\n\n\tplotOptions: {\n\t\tseries: {\n\t\t\tpoint: {\n\t\t\t\tevents: {\n\t\t\t\t\tclick: function () {\n\t\t\t\t\t\tlet str = 'https://nv2.fandom.com/wiki/';\n\t\t\t\t\t\tif (Number(this.options.x) > 109)\n\t\t\t\t\t\t\t"
                        "str = str + 'B' + Number(this.options.x) %% 10;\n\t\t\t\t\t\telse if (Number(this.options.x) > 99)\n\t\t\t\t\t\t\tstr = str + 'A' + Number(this.options.x) %% 10;\n\t\t\t\t\t\telse if (Number(this.options.x) < 10)\n\t\t\t\t\t\t\t"
                        "str = str + '0' + Number(this.options.x);\n\t\t\t\t\t\telse\n\t\t\t\t\t\t\tstr = str + this.options.x;\n\t\t\t\t\t\tstr = str + '-' + this.options.y;\n\t\t\t\t\t\twindow.location.href = str;\n\t\t\t\t\t"
                        "}\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\t},\n\n\tseries: [{\n\t\tname: 'Position per level',\n\t\tborderWidth: 1,\n\t\tdata: [" };

//! Hard template for histogram chart on heatmap page
static const char *histogram_chart = { "Highcharts.chart('container2', {\n\tchart: {\n\t\ttype: 'column',\n\t},\n\ttitle: {\n\t\ttext: 'Positions histogram'\n\t},\n\txAxis: {\n\t\tcategories: ['0th', '1st', '2nd', '3rd', '4th', '5th', '6th', '7th', '8th', '9th']"
                                "\n\t},\n\tyAxis: {\n\t\tmin: 0,\n\t\ttitle: {\n\t\t\ttext: 'Positions histogram'\n\t\t},\n\t\tstackLabels: {\n\t\t\tenabled: true,\n\t\t\tstyle: {\n\t\t\t\tfontWeight: 'bold',\n\t\t\t\tcolor: ( //theme"
                                "\n\t\t\t\tHighcharts.defaultOptions.title.style && Highcharts.defaultOptions.title.style.color) || 'gray'\n\t\t\t}\n\t\t}\n\t},\n\tlegend: {\n\t\talign: 'right',\n\t\tx: -30,\n\t\tverticalAlign: 'top',\n\t\ty: 25,\n\t\t"
                                "backgroundColor: Highcharts.defaultOptions.legend.backgroundColor || 'white',\n\t\tborderColor: '#CCC',\n\t\tborderWidth: 1,\n\t\tshadow: false\n\t},\n\ttooltip: {\n\t\theaderFormat: '<b>{point.x}</b><br/>',\n\t\t"
                                "pointFormat: '{series.name}: {point.y}<br/>'\n\t},\n\tplotOptions: {\n\t\tcolumn: {\n\t\t\tpointPadding: 0,\n\t\t\tborderWidth: 0,\n\t\t\tgroupPadding: 0,\n\t\t\tstacking: 'normal',\n\t\t\tdataLabels: {\n\t\t\t\t"
                                "enabled: false\n\t\t\t}\n\t\t}\n\t},\n\tseries: [{\n\t\tname: 'Number of positions',\n\t\tdata: [" };

//! Timeline charts file names
const char *chart_files[] = {"0thRankingsChart.htm", "0thRankingsNoTiesChart.htm", "Top10RankingsChart.htm", "Top5RankingsChart.htm", "PointsRankingsChart.htm", "AverageRankingsChart.htm"};
//! Name placeholder used in callback functions called by sqlite
const char *name_placeholder = { "\t\tname: '%s',\n\t\tdata: [" };
//! Date placeholder + integer data used in callback functions called by sqlite
const char *date_placeholder = { "\t\t\t[Date.UTC(%4d,%2d,%2d),%5d]" };
//! Date placeholder + fractional data used in callback functions called by sqlite
const char *date_placeholder_float = { "\t\t\t[Date.UTC(%4d,%2d,%2d),%.6f]" };
//! Date placeholder + nil data used in callback functions called by sqlite
const char *date_placeholder_null = { "\t\t\t[Date.UTC(%4d,%2d,%2d), null]" };

//! Buffer used for forming SQL commands
#define SQL_CMD_BUF_LEN 512
static char sql_cmd[SQL_CMD_BUF_LEN];

//! Buffer used for forming big strings used for generating charts
char form_buffer[1000];

//! Global file handling object for writing
static FILE *fw;
//! Global buffer used for R/W
static char buffer[255];


static const char *players[] = {"EddyMataGallos", "kkstrong", "VotedStraw", "SpartaX18", "oxymoron93", "shomman",
                         "cooperverdon", "jp27ace", "danielr", "Oleary15", "White Whale", "Polt", "V3L0C1T4",
                         "swipenet", "shilo1122", "Thisguy248", };
#define NUM_PLAYERS     (sizeof players/ sizeof *players)      // Number of players for which timeline charts are generated

//! Some colors in HEX format for some CSS stylings
static const char *colors[] = { "#BE3075",  "#EB001F",  "#64A12D",  "#000000",  "#00DDEE",  "#008AC5",  "#a83232",  "#005555",  "#FFED00",  "#FFEDAA",
                         "#ABCDEF",  "#bb00bb",  "#888888",  "#325ea8",  "#9a32a8",  "#00aa00",  "#a83277",  "#32a85f",  "#009EE0",  "#83a832",
};

//! SQLITE BEGIN and END transaction commands
static const char *begin_transaction = { "BEGIN TRANSACTION" };
static const char *end_transaction = { "END TRANSACTION" };

//! Command for downloading level leaderboard
static const char *download_scores_template = { "wget -q -N http://REDACTED/%1u/%02u/%03u.txt 2>nul" };

//! Command for downloading demo data
static const char *download_demos_template = { "wget -q http://REDACTED/%s 2>nul" };

//! Scores by level and position
unsigned loadScores[NUM_LEVELS][NUM_ENTRIES];

//! Player nicks by level and position
char loadNames[NUM_LEVELS][NUM_ENTRIES][MAX_NAME_LEN + 1];

//! ID's by level and position
unsigned loadIDs[NUM_LEVELS][NUM_ENTRIES];

//! Old scores by level and position
unsigned oldScores[NUM_LEVELS][NUM_ENTRIES];

//! Old player nicks by level and position
char oldNames[NUM_LEVELS][NUM_ENTRIES][MAX_NAME_LEN + 1];

//! Global variable for determining current row given by SQLITE procedures
static unsigned int row_cnt;

//! Global variable used in callbacks called by SQLITE procedures
static int prev_score;

//! Global variable which stored number of distinct days for given stats
static unsigned int distinct_days;

//! Flag which indicates some nil condition
static int flag_null;

//! This one stores everything you can imagine, description in function used
static int generic_int_placeholder;

#define HIST_LEN    10          // Length of the histogram array, used for players spots on leaderboard
static unsigned short histogram[HIST_LEN];

/** ************************************************************************************************
 *  \brief        getYesterdayDate
 *                Takes date structure and decreses it by one day
 *
 *  \param [in]   struct date *d
 *
 *  \return [out] None
 */
static void getYesterdayDate(struct date *d){
	if(d->dd==1){
		if(d->mm==2||d->mm==4||d->mm==6|| d->mm==8||d->mm==9||d->mm==11){
			d->dd=31;
			d->mm--;
		}
		else if(d->mm==3){
			if((d->yy % 400 == 0) || ((d->yy%4 == 0) && (d->yy % 100 != 0))) d->dd=29;
			else d->dd=28;
			d->mm--;
		}
		else if(d->mm==1){
			d->dd=31;
			d->mm=12;
			d->yy--;
		}
		else{
			d->dd=30;
			d->mm--;
		}
	}
	else{
		d->dd--;
	}
}

/** ************************************************************************************************
 *  \brief        getTomorrowDate
 *                Takes date structure and increases it by one day
 *
 *  \param [in]   struct date *d
 *
 *  \return [out] None
 */
static void getTomorrowDate(struct date* d) {
    if (d->dd == 31) {
        d->dd = 1;
        if (d->mm == 12) {
            d->mm = 1;
            d->yy++;
        }
        else {
            d->mm++;
        }
    }
    else if (d->dd == 30) {
        if (d->mm == 4 || d->mm == 6 || d->mm == 9 || d->mm == 11) {
            d->dd = 1;
            d->mm++;
        }
        else {
            d->dd = 31;
        }
    }
    else if (d->dd == 29) {
        if (d->mm == 2) {
            d->dd = 1;
            d->mm = 3;
        }
        else {
            d->dd = 30;
        }
    }
    else if (d->dd == 28) {
        if (d->mm == 2 && (!((d->yy % 400 == 0) || ((d->yy % 4 == 0) && (d->yy % 100 != 0))))) {
            d->dd = 1;
            d->mm = 3;
        }
        else {
            d->dd = 29;
        }
    }
    else {
        d->dd++;
    }
}

static bool equal_date(const struct date d1, const struct date d2)
{
    if (d1.yy == d2.yy && d1.mm == d2.mm && d1.dd == d2.dd) return true;
    else return false;
}

#if 1 //DATABASE QUERY CALLBACKS

/** ************************************************************************************************
 *  \brief        callback_count
 *                Stores result returned by count aggregate function used in various sql commands
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_count(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;
    long count = strtol(*row, NULL, 10);;

    SQLITE_INT_RANGE_CHECK(count);

    *(int *)storage = (int) count;

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        gen_history_callback
 *                Fills up the storage data structure with result of sql query about 0ths changes
 *                Used by \function generate_level_history
 *                Uses \a gen_history_cb_cnt global to inform about rows ordinal number
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static unsigned int gen_history_cb_cnt;
static int gen_history_callback(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;

    struct storage *p = storage;

    long score = strtol(row[2], NULL, 10);

    SQLITE_INT_RANGE_CHECK(score);

    assert(strlen(row[0]) < 11);
    strncpy(p[gen_history_cb_cnt].day, row[0], 10);
    p[gen_history_cb_cnt].day[10] = 0;
    strncpy(p[gen_history_cb_cnt].nick, row[1], MAX_NAME_LEN);
    p[gen_history_cb_cnt].nick[MAX_NAME_LEN] = 0;
    p[gen_history_cb_cnt++].score = (int)score;

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_oneline
 *                Writes one line of formatted output to files used by timeline charts
 *                Used by \function update_charts
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_oneline(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;
    long  year, month, day;
    struct tm* t = (struct tm*)storage;

    year = strtol(row[0], NULL, 10);
    month = strtol(&row[0][5], NULL, 10) - 1;
    day = strtol(&row[0][8], NULL, 10);

    SQLITE_INT_RANGE_CHECK(year);
    SQLITE_INT_RANGE_CHECK(month);
    SQLITE_INT_RANGE_CHECK(day);

    if (t->tm_wday == TYPE_INT) {
        long count = strtol(row[1], NULL, 10);

        SQLITE_INT_RANGE_CHECK(count);

        if (count) {
            fprintf(fw, date_placeholder, (int)year, (int)month, (int)day, (int)count);
            flag_null = 0;
        }
        else {
            if (flag_null)
                fprintf(fw, date_placeholder_null, (int)year, (int)month, (int)day);
            else
                fprintf(fw, date_placeholder,  (int)year, (int)month, (int)day, 0);
            flag_null = 1;
        }
    }
    else {
        double count = strtod(row[1], NULL);
        fprintf(fw, date_placeholder_float, (int)year, (int)month, (int)day, count);
        flag_null = 0;
    }
    if (row_cnt++ == distinct_days || ((1900 + t->tm_year == year) && (t->tm_mon == month) && (t->tm_mday == day)))
        fprintf(fw, "\n");
    else
        fprintf(fw, ",\n");
    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_oneline
 *                Stores nicknames taken by sql query for 0ths changes history on a level
 *                Used by \function generate_level_history
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int distinct_nick_cb_cnt;
static int distinct_nick_callback(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;

    char (*names)[MAX_NAME_LEN + 1] = storage;

    strncpy(names[distinct_nick_cb_cnt++], *row, MAX_NAME_LEN + 1);

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        distinc_scores_callback
 *                Writes level history data about difference between 0th and 1st on particular level
 *                Used by \function generate_diff_level_history
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int distinc_scores_callback(void *storage, int argc, char **row, char **columns)
{
    (void)storage;
    (void)argc;
    (void)columns;
    struct date yesterday, today;

    today.yy = (int)strtol(row[0], NULL, 10);
    today.mm = (int)strtol(&row[0][5], NULL, 10);
    today.dd = (int)strtol(&row[0][8], NULL, 10);
    if (row_cnt++) {
        yesterday = today;
        getYesterdayDate(&yesterday);
        printf(date_placeholder, yesterday.yy, yesterday.mm - 1, yesterday.dd, prev_score);
        printf(",\n");
    }
    prev_score = (int)strtol(row[1], NULL, 10);
    printf(date_placeholder, today.yy, today.mm - 1, today.dd, prev_score);
    printf(",\n");

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        activity_callback
 *                Writes level history data about difference between 0th and 1st on particular level
 *                Used by \function generate_activity
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int activity_callback(void* storage, int argc, char** row, char** columns)
{
    (void)storage;
    (void)argc;
    (void)columns;
    struct date yesterday, now;
    static struct date prev;

    now.yy = (int)strtol(row[0], NULL, 10);
    now.mm = (int)strtol(&row[0][5], NULL, 10);
    now.dd = (int)strtol(&row[0][8], NULL, 10);
    
    if (row_cnt++) {
        yesterday = now;
        getYesterdayDate(&yesterday);
        if (!equal_date(yesterday, prev)) {
            struct date tomorrow = prev;
            getTomorrowDate(&tomorrow);
            printf(date_placeholder, tomorrow.yy, tomorrow.mm - 1, tomorrow.dd, 0);
            printf(",\n");
            if (!equal_date(yesterday, tomorrow)) {
                printf(date_placeholder, yesterday.yy, yesterday.mm - 1, yesterday.dd, 0);
                printf(",\n");
            }
        }
    }

    long count = strtol(row[1], NULL, 10);
    SQLITE_INT_RANGE_CHECK(count);
    printf(date_placeholder, now.yy, now.mm - 1, now.dd, (int)count);
    printf(",\n");
    prev = now;

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_day
 *                Stores date as string in format YYYY-MM-DD given back as result from sql commands
 *                Used by \function set_tied_scores
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_day(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;
    strncpy(storage, *row, 11);

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_show_level
 *                Prints out the metanet level id in form xx-y given back as result of sql command
 *                Used by \function show_older_0ths
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_show_level(void *storage, int argc, char **row, char **columns)
{
    (void)storage;
    (void)argc;
    (void)columns;
    printf("%02d-%c ", (int)strtol(row[0], NULL, 10), row[1][0]);

    // global variable, in here it keeps track of row count, so we can do new-line formatting
    if (row_cnt++ % 5 == 4) putchar('\n');

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_histogram
 *                Fills up \e histogram array object with data used for processing histogram graph
 *                Used by \function generate_heatmap and \function generate_histogram
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_histogram(void *storage, int argc, char **row, char **columns)
{
    (void)storage;
    (void)argc;
    (void)columns;

    long hist_val = strtol(row[1], NULL, 10);

    SQLITE_USHRT_INT_RANGE_CHECK(hist_val);

    histogram[row[0][0] - '0'] = (unsigned short)hist_val;
    if (histogram[row[0][0] - '0'] > generic_int_placeholder)
        generic_int_placeholder = histogram[row[0][0] - '0'];

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_fill_leaderboard
 *                Stores nickname for particular metanet level and place. Used to fill out
 *                leaderboards with nicknames.
 *                Used by \function generate_changes_list
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_fill_leaderboard(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;

    char (*names)[NUM_ENTRIES][MAX_NAME_LEN + 1] = storage;
    int level = (int)strtol(row[1], NULL, 10);
    int place = (int)strtol(row[2], NULL, 10);

    strncpy(names[level][place], row[0], MAX_NAME_LEN + 1);

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_heatmap
 *                Prints out data necessary for generating heatmap charts.
 *                Used by \function generate_heatmap
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_heatmap(void *storage, int argc, char **row, char **columns)
{
    (void)storage;
    (void)argc;
    (void)columns;

    static int lvl = 0;
    int curr_level = (int)strtol(row[0], NULL, 10);
    int place = (int)strtol(row[1], NULL, 10);

    // generic in placeholder represents number of placements player has on metanet level boards, it is strictly non-negative.
    assert(generic_int_placeholder >= 0);
    const unsigned int num_of_placements = (unsigned) generic_int_placeholder;
    
    while (lvl < 600) {
        if (lvl != curr_level) {
            fprintf(fw, "[%d, %d, 10]", lvl/ 5, lvl% 5);
            if (lvl != 599)
                fprintf(fw, ", ");
            if (lvl%13 == 12)
                fprintf(fw, "\n\t\t\t");
            ++lvl;
        }
        else {
            fprintf(fw, "[%d, %d, %d]", lvl/ 5, lvl% 5, place);
            if (lvl != 599)
                fprintf(fw, ", ");
            if (lvl%13 == 12)
                fprintf(fw, "\n\t\t\t");
            ++lvl;
            if (++row_cnt != num_of_placements)
                break;
        }
    }

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_stats
 *                Fills out the stats returned by sql command.
 *                Used by \function update_charts
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_stats(void* storage, int argc, char** row, char** column)
{
    (void)column;

    struct sql_callback_data* out = (struct sql_callback_data*)storage;

    for (int i = 0; i < argc; i++) {
        switch (out->format[i]) {
        case '1': snprintf(out->stats[out->stats_cnt].nick, sizeof out->stats[out->stats_cnt].nick, "%s", row[i]); break;
        case '2': out->stats[out->stats_cnt].ties = (int)strtol(row[i], NULL, 10); break;
        case '3': out->stats[out->stats_cnt].pure = (int)strtol(row[i], NULL, 10); break;
        case '4': out->stats[out->stats_cnt].top10s = (int)strtol(row[i], NULL, 10); break;
        case '5': out->stats[out->stats_cnt].top5s = (int)strtol(row[i], NULL, 10); break;
        case '6': out->stats[out->stats_cnt].pts = (int)strtol(row[i], NULL, 10); break;
        default: fprintf(stderr, "Err in func: [%s]", __func__); break;
        }
    }

    out->stats_cnt++;
    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_latest_0ths
 *                Prints out the metanet level followed by a new 0th holder.
 *                Used by \function generate_latest_0ths
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_latest_0ths(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;
    (void)storage;

    fprintf(fw, "\t\t<li>%02d-%c<br>%s:+%d<br>\n\t\t%s</li><hr>\n", 
        (int)strtol(row[0], NULL, 10), row[1][0], row[2], (int)strtol(row[3], NULL, 10), row[4]);

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_leaderboard
 *                Prints out the one entry in the level's leaderboard as it looks in the game.
 *                Used by \function get_leaderboard
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_leaderboard(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;
    (void)storage;

    fprintf(stdout, "%c|%-20s|%6s\n", row[0][0], row[1], row[2]);

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_ownage
 *                Prints out players having episode ownage (0th on all 5 levels belonging to
 *                episode) with corresponding episode.
 *                Used by \function show_episode_ownages
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_ownage(void *storage, int argc, char **row, char **columns)
{
    (void)argc;
    (void)columns;
    (void)storage;

    fprintf(stdout, "episode %02d %s\n", (int)strtol(row[0], NULL, 10), row[1]);

    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        callback_board_diff
 *
 *                Used by \function show_board_diff_inner
 *
 *  \param [in]   void * storage
 *  \param [in]   int argc
 *  \param [in]   char **row
 *  \param [in]   char **columns
 *
 *  \return [out] SQLITE_OK
 */
static int callback_board_diff(void *storage, int argc, char **row, char **column)
{
    (void)storage;
    (void)argc;
    (void)column;

    fprintf(stdout, "%3s-%s: %-5s (%s)\n", row[0], row[1], row[2], row[3]);
    return SQLITE_OK;
}

#endif // 1

/** ************************************************************************************************
 *  \brief        print_histogram
 *                Prints out histogram for a player to the console.
 *                Used by \function generate_histogram
 *
 *  \param [in]   const char *player - A player nickname string
 *
 *  \return [out] SQLITE_OK
 */
static void print_histogram(const char *player)
{
    int k;

    printf("Scores histogram for player [%s]:\n\n", player);

    k = (int)round(generic_int_placeholder /10.);

    for (size_t i = 0; i < HIST_LEN; ++i)
        histogram[i] = (unsigned short)round(histogram[i] /10.);

    for (int i = k; i > 0; --i) {
        printf("%3d. |", i * 10);
        for (size_t j = 0; j < HIST_LEN; ++j) {
            if (i == histogram[j]) {
                printf(" * ");
                --histogram[j];
            }
            else printf("   ");
        }
        putchar('\n');
    }
    printf("-----------------------------------\n");
    printf("     | 0  1  2  3  4  5  6  7  8  9  \n");
}

#if 1 // COMPARISON FUNCTIONS
static int cmp_pure(const void *v1, const void *v2)
{
    return ((struct stats *)v2)->pure - ((struct stats *)v1)->pure;
}

static int cmp_top10s(const void *v1, const void *v2)
{
    int cmp_val = (*(struct stats **)v2)->top10s - (*(struct stats **)v1)->top10s;
    return cmp_val ? cmp_val : ((*(struct stats **)v2)->top5s - (*(struct stats **)v1)->top5s);
}

static int cmp_pts(const void *v1, const void *v2)
{
    return (*(struct stats **)v2)->pts - (*(struct stats **)v1)->pts;
}

static int cmp_str(const void *v1, const void *v2)
{
    return strcmp(*(const char **)v1, *(const char **)v2);
}
#endif // 1

/* ============================================================================================== */
/*                                            Interface                                           */
/* ============================================================================================== */

int create_table(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    char *name = *argv;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return -1;
    }

    snprintf(sql_cmd, sizeof sql_cmd, "DROP TABLE IF EXISTS %s;", name);
    rc = sqlite3_exec(db, sql_cmd, NULL, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return -2;
    }

    snprintf(sql_cmd, sizeof sql_cmd, 
        "CREATE TABLE %s(day DATE, level INT(3) NOT NULL, nick VARCHAR(255) NOT NULL, place INT(1) NOT NULL, score INT(10) NOT NULL, flag_tied INT(1) NOT NULL, PRIMARY KEY(day, level, place)) WITHOUT ROWID;", name);
    rc = sqlite3_exec(db, sql_cmd, NULL, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return -3;
    }

    sqlite3_close(db);
    return SQLITE_OK;
}

int insert_into(int argc, char **argv, struct tm *t)
{
    (void)t;

    sqlite3 *db;                            // sqlite3 database handler
    char *zErrMsg = NULL;                   // pointer to message stored by sqlite functions
    int rc;                                 // result code of function calls

    rc = sqlite3_open("highscores.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rc = sqlite3_exec(db, begin_transaction, NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

#if 0
    if (flag == FLAG_DOWNLOAD) {
        for (int i = 0; i < NUM_LEVELS; ++i) {
            for (int j = 0; j < NUM_ENTRIES; ++j) {
                int count = 0;
                if (j != 0 && loadScores[i][0] == loadScores[i][j])
                    snprintf(sql_cmd, sizeof sql_cmd, insert, "scores", 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, i, loadNames[i][j], j, loadScores[i][j], ",'1'");
                else
                    snprintf(sql_cmd, sizeof sql_cmd, insert, "scores", 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, i, loadNames[i][j], j, loadScores[i][j], ",'0'");
                rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &zErrMsg);
                if (rc) {
                    fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    sqlite3_close(db);
                    return -7;
                }

                snprintf(sql_cmd, sizeof sql_cmd, check_score, "scores2", i, j, loadNames[i][j], loadScores[i][j]);
                rc = sqlite3_exec(db, sql_cmd, callback_count, &count, &zErrMsg);
                if (rc) {
                    fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    sqlite3_close(db);
                    return -8;
                }

                if (count) continue;

                snprintf(sql_cmd, sizeof sql_cmd, insert, "scores2", 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, i, loadNames[i][j], j, loadScores[i][j], "");
                printf("%s\n", sql_cmd);
                rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &zErrMsg);
                if (rc) {
                    fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    sqlite3_close(db);
                    return -9;
                }
            }
        }
    }
    else if (flag == FLAG_HS) {
#endif
        // reorder hs files by chronological order
        qsort(argv, (size_t)argc, sizeof *argv, cmp_str);

        // read each hs and insert it
        for ( ; *argv; ++argv) {
            FILE *fr = NULL;
            int c;
            char buf[256];
            int day, month, year;

            if (fr = fopen(*argv, "r"), fr == NULL) {
                fprintf(stderr, "Error in opening highscore file %s\n", *argv);
                sqlite3_close(db);
                return -3;
            }

            if (fgets(buf, sizeof buf, fr) == NULL) {
                fprintf(stderr, "Error in reading file %s!\n", *argv);
                sqlite3_close(db);
                fclose(fr);
                return -4;
            }
            if (strcmp(buf, "#*N2High-v3.0*#\n")) {
                fprintf(stderr, "Header suggests this is not highscore file! %s\n", *argv);
                sqlite3_close(db);
                fclose(fr);
                return -5;
            }

            day = (int)strtoul(&argv[0][6], NULL, 10);
            argv[0][6] = 0;
            month = (int)strtoul(&argv[0][4], NULL, 10);
            argv[0][4] = 0;
            year = (int)strtoul(argv[0], NULL, 10);

            for (int i = 0; i < NUM_LEVELS; ++i) {
                for (int j = 0; j < NUM_ENTRIES; ++j) {
                    size_t len;
                    char input[33];
                    char output[33];

                    // reading name
                    len = 0;
                    while ((c = fgetc(fr)) != EOF && c != '\'')
                        input[len++] = (char)c;
                    input[len] = 0;
                    cypher(input);
                    base64_decode(input, len, (unsigned char *)output, OUTPUT_LEN_IGNORED);
                    strcpy(loadNames[i][j], output);

                    // reading score
                    len = 0;
                    while ((c = fgetc(fr)) != EOF && c != '\'')
                        input[len++] = (char)c;
                    input[len] = 0;
                    cypher(input);
                    base64_decode(input, len, (unsigned char *)output, OUTPUT_LEN_IGNORED);
                    loadScores[i][j] = (unsigned)strtoul(output, NULL, 10);

                    // This one is ignored (the ID of the player)
                    len = 0;
                    while ((c = fgetc(fr)) != EOF && c != '\'')
                        input[len++] = (char)c;
                    input[len] = 0;
                    cypher(input);
                    base64_decode(input, len, (unsigned char *)output, OUTPUT_LEN_IGNORED);
                    loadIDs[i][j] = (unsigned)strtoul(output, NULL, 10);

#define SQL_INSERT_CMD "INSERT INTO %s VALUES('%04d-%02d-%02d','%d','%s','%d','%u'%s);"

                    int count = 0;
                    if (j != 0 && loadScores[i][0] == loadScores[i][j])
                        snprintf(sql_cmd, sizeof sql_cmd, SQL_INSERT_CMD, "scores", year, month, day, i, loadNames[i][j], j,
                                 loadScores[i][j], ",'1'");
                    else
                        snprintf(sql_cmd, sizeof sql_cmd, SQL_INSERT_CMD, "scores", year, month, day, i, loadNames[i][j], j,
                                 loadScores[i][j], ",'0'");

                    rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &zErrMsg);
                    if (rc) {
                        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        sqlite3_close(db);
                        fclose(fr);
                        return -6;
                    }

                    snprintf(sql_cmd, sizeof sql_cmd, 
                        "SELECT count(*) FROM %s WHERE level=%d AND place=%d AND nick='%s' AND score=%u;", "scores2", i, j, loadNames[i][j], loadScores[i][j]);
                    rc = sqlite3_exec(db, sql_cmd, callback_count, &count, &zErrMsg);
                    if (rc) {
                        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        sqlite3_close(db);
                        fclose(fr);
                        return -7;
                    }

                    if (count) continue;

                    snprintf(sql_cmd, sizeof sql_cmd, SQL_INSERT_CMD, "scores2", year, month, day, i, loadNames[i][j], j,
                             loadScores[i][j], "");
                    printf("%s\n", sql_cmd);
                    rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &zErrMsg);
                    if (rc) {
                        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        sqlite3_close(db);
                        fclose(fr);
                        return -8;
                    }
                }
            }

            fclose(fr);
        }
#if 0
    }
#endif

    rc = sqlite3_exec(db, end_transaction, NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -9;
    }

    sqlite3_close(db);
    return SQLITE_OK;
}

int count_player_X_place(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    int place = (*argv++)[0] - '0';
    char player[MAX_NAME_LEN] = {0};

    strcat(player, *argv++);
    while (*argv) {
        player[strlen(player)] = ' ';
        strcat(player, *argv++);
    }

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    int *count = &(int){0};

    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT count(*) FROM (SELECT * FROM scores ORDER BY day DESC LIMIT 6000) AS sub WHERE sub.nick='%s' AND sub.place=%d;", player, place);
    rc = sqlite3_exec(db, sql_cmd, callback_count, count, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      sqlite3_close(db);
      return -2;
    }

    const char* ord = (place == 1) ? "st" :
                      (place == 2) ? "nd" :
                      (place == 3) ? "rd" : "th";
    printf("Number of %d-%s spots for player [%s] is: %d\n", place, ord, player, *count);

    sqlite3_close(db);
    return SQLITE_OK;
}

int count_top10s(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    int count = -1;
    snprintf(sql_cmd, sizeof sql_cmd,
             "SELECT count(*) FROM (SELECT nick FROM scores ORDER BY day DESC LIMIT 6000) as stbl where stbl.nick='%s';", *argv);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error executing command %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    fprintf(stdout, "Number of top10 scores for [%s] is %d\n\n", *argv, count);

    sqlite3_close(db);
    return SQLITE_OK;
}

int count_top5s(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    int count = -1;
    snprintf(sql_cmd, sizeof sql_cmd,
             "SELECT count(*) FROM (SELECT nick, place FROM scores ORDER BY day DESC LIMIT 6000) as stbl where stbl.nick='%s' AND stbl.place<5;", *argv);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error executing command %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    fprintf(stdout, "Number of top5 scores for [%s] is %d\n\n", *argv, count);

    sqlite3_close(db);
    return SQLITE_OK;
}

/** ************************************************************************************************
 *  \brief        download_score_file
 *                Downloads highscore file for given level id
 *
 *  \param [in]   int level
 *
 *  \return [out] None
 */
static void download_score_file(unsigned level)
{
    char cmd[100];
    snprintf(cmd, sizeof cmd, download_scores_template, level /100000, (level %100000) /1000, level %1000);
    IGNORE_RESULT(system(cmd));
}

static int parse_score_file(unsigned level, char (*names)[MAX_NAME_LEN + 1], unsigned* ids, unsigned* scores, int entries)
{
    (void)entries;

    unsigned char buf[512] = { 0 };
    char filename[8];
    FILE* bfr;
    int buf_size = 0;
    int rank = -1;
    int name_len = 0;

    snprintf(filename, sizeof filename, "%03u.txt", level %1000);
    if (bfr = fopen(filename, "rb"), bfr == NULL) {
        printf("Error reading file %s", filename);
        return 1;
    }
    for (int c; (c = fgetc(bfr)) != EOF; ) buf[buf_size++] = (unsigned char)c;
    fclose(bfr);

    // disregard last new-line character
    buf_size--;
    for (int j = 0; j < buf_size; ++j) {
        if (buf[j] < 0x20) {
            char score_str[7];
            char id_str[9];
            /*s[1] = "0123456789ABCDEF"[buf[j + 1] % 16];
            buf[j + 1] /= 16;
            s[0] = "0123456789ABCDEF"[buf[j + 1] % 16];
            s[3] = "0123456789ABCDEF"[buf[j + 2] % 16];
            buf[j + 2] /= 16;
            s[2] = "0123456789ABCDEF"[buf[j + 2] % 16];
            s[4] = 0;*/
            rank++;
            name_len = 0;
            snprintf(score_str, sizeof score_str, "%02X%02X%02X", buf[j], buf[j + 1], buf[j + 2]);
            scores[rank] = (unsigned)strtoul(score_str, NULL, 16);
            snprintf(id_str, sizeof id_str, "%02X%02X%02X%02X", buf[j + 3], buf[j + 4], buf[j + 5], buf[j + 6]);
            ids[rank] = (unsigned)strtoul(id_str, NULL, 16);
            j += 7;
        }
        assert(buf[j] <= SCHAR_MAX);
        names[rank][name_len++] = (char)buf[j];
    }

    return 0;
}

int download_level_scores(int argc, char** argv, struct tm* t)
{
    (void)argc;
    (void)t;

    char names[NUM_ENTRIES][MAX_NAME_LEN + 1] = { 0 };
    unsigned ids[NUM_ENTRIES] = { 0 };
    unsigned scores[NUM_ENTRIES] = { 0 };
    int level = (int)strtol(argv[0], NULL, 10);

    if (level < 0)
    {
        return -1;
    }

    download_score_file((unsigned)level);
    
    int rc = parse_score_file((unsigned)level, names, ids, scores, NUM_ENTRIES);

    if (rc != 1) {
        for (int i = 0; i < NUM_ENTRIES; i++) {
            printf("%d.| [%31s]%6u%7u\n", i, names[i], scores[i], ids[i]);
        }
    }
    else {
        printf("Issue while trying to read %d file, it may be file is not downloaded due unexistant internet connection!\n", level % 1000);
    }

    return 0;
}

int download_metanet_scores(int argc, char** argv, struct tm* t)
{
    (void)argc;
    (void)argv;

    char buffer_in[256] = { 0 };
    char buffer_out[72] = { 0 };
    size_t len;
    char* time_buf = NULL;
    FILE* bfr;
    int parse_cnt = 0;

    for (unsigned i = 0; i < NUM_LEVELS; ++i) {
        download_score_file(i + 1);
    }

    for (unsigned i = 0; i < NUM_LEVELS; ++i) {
        parse_cnt += parse_score_file(i + 1, loadNames[i], loadIDs[i], loadScores[i], NUM_ENTRIES);
    }

    if (parse_cnt != 0) {
        printf("Issue with downloading/parsing score files!\n");
        return 1;
    } 
    else {

        time_buf = asctime(t);
        time_buf[strcspn(time_buf, "\n")] = 0;

        snprintf(buffer_in, sizeof buffer_in, "%4d%02d%02d.hs2", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        if (bfr = fopen(buffer_in, "w"), bfr == NULL) {
            printf("Couldn't create .hs2 file!\n");
            return -5;
        }

        fprintf(bfr, "#*N2High-v3.0*#\n");
        for (int i = 0; i < NUM_LEVELS; ++i) {
            for (int j = 0; j < NUM_ENTRIES; ++j) {
                len = (size_t)snprintf(buffer_in, sizeof buffer_in, "%s", loadNames[i][j]);
                base64_encode((unsigned char *)buffer_in, len, buffer_out, OUTPUT_LEN_IGNORED);
                cypher(buffer_out);
                fprintf(bfr, "%s'", buffer_out);

                len = (size_t)snprintf(buffer_in, sizeof buffer_in, "%u", loadScores[i][j]);
                base64_encode((unsigned char *)buffer_in, len, buffer_out, OUTPUT_LEN_IGNORED);
                cypher(buffer_out);
                fprintf(bfr, "%s'", buffer_out);

                len = (size_t)snprintf(buffer_in, sizeof buffer_in, "%u", loadIDs[i][j]);
                base64_encode((unsigned char *)buffer_in, len, buffer_out, OUTPUT_LEN_IGNORED);
                cypher(buffer_out);
                fprintf(bfr, "%s'", buffer_out);
            }
        }

        len = (size_t)snprintf(buffer_in, sizeof buffer_in, "%s", time_buf);
        base64_encode((unsigned char *)buffer_in, len, buffer_out, OUTPUT_LEN_IGNORED);
        cypher(buffer_out);
        fprintf(bfr, "%s", buffer_out);

        fclose(bfr);

        return 0;
    }
}

int generate_level_history(int argc, char **argv, struct tm *t)
{
    (void)argc;
    sqlite3 *db;
    int count[2] = {0};
    int rc;
    int res = SQLITE_OK;
    int year = -1, month = -1, day = -1;
    char *zErrMsg;
    struct storage *changes = NULL;
    char (*names)[MAX_NAME_LEN + 1];
    struct date yesterday;
    int max_score = -1;
    char max_name[MAX_NAME_LEN + 1];

    unsigned lvl = str_to_lvl(*argv);

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }


    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT count(%snick) FROM scores2 WHERE level=%u AND place=0;", "", lvl);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &count[0], &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        res = -2;
        goto sqlite3_cleanup;
    }

    assert(count[0] > 0);
    if (changes = malloc((size_t)count[0] * sizeof *changes), changes == NULL) {
        fprintf(stderr, "Dynamic allocation error for changes... \n");
        res = -3;
        goto sqlite3_cleanup;
    }

    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT count(%snick) FROM scores2 WHERE level=%u AND place=0;", "DISTINCT ", lvl);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &count[1], &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        res = -4;
        goto changes_cleanup;
    }

    assert(count[1] > 0);
    if (names = malloc((size_t)count[1] * sizeof *names), names == NULL) {
        fprintf(stderr, "Dynamic allocation error for names... \n");
        res = -5;
        goto changes_cleanup;
    }

    gen_history_cb_cnt = 0;
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT day, nick, score FROM scores2 WHERE level=%u AND place=%d;", lvl, 0);
    rc = sqlite3_exec(db, sql_cmd, gen_history_callback, changes, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        res = -6;
        goto names_cleanup;
    }

    max_score = changes[count[0] - 1].score;
    strncpy(max_name, changes[count[0] - 1].nick, MAX_NAME_LEN + 1);
    max_name[MAX_NAME_LEN] = '\0';

    distinct_nick_cb_cnt = 0;
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT %snick FROM scores2 WHERE level=%u and place=%d;", "DISTINCT ", lvl, 0);
    rc = sqlite3_exec(db, sql_cmd, distinct_nick_callback, names, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        res = -7;
        goto names_cleanup;
    }

    snprintf(form_buffer, sizeof form_buffer, highchart, lvl, "none", lvl, lvl/5, lvl - lvl/5*5, changes[0].score - 10);
    printf("%s", form_buffer);

    for (int i = 0; i < count[1]; ++i) {
        int flag = 0;
        printf(name_placeholder, names[i]);
        for (int j = 0; j < count[0]; ++j) {
            yesterday.yy = year = (int)strtol(changes[j].day, NULL, 10);
            yesterday.mm = month = (int)strtol(&changes[j].day[5], NULL, 10);
            yesterday.dd = day = (int)strtol(&changes[j].day[8], NULL, 10);
            getYesterdayDate(&yesterday);

            if (strncmp(names[i], changes[j].nick, MAX_NAME_LEN + 1) == 0) {
                if (j == 0 || flag == 2){
                    printf("\n");
                    printf(date_placeholder, year, month - 1, day, changes[j].score);
                    if (flag) flag = 0;
                }
                else {
                    printf(",\n");
                    if (strncmp(names[i], changes[j - 1].nick, MAX_NAME_LEN + 1) == 0)
                        printf(date_placeholder, yesterday.yy, yesterday.mm - 1, yesterday.dd, changes[j - 1].score);
                    else {
                        printf(date_placeholder_null, yesterday.yy, yesterday.mm - 1, yesterday.dd);
                        flag = 0;
                    }
                    printf(",\n");
                    printf(date_placeholder, year, month - 1, day, changes[j].score);
                }
            }
            else if (flag == 0){
                if (j == 0)
                    flag = 2;
                else {
                    printf(",\n");
                    printf(date_placeholder, yesterday.yy, yesterday.mm - 1, yesterday.dd, changes[j - 1].score);
                    flag = 1;
                }
            }
        }
        if (strncmp(max_name, names[i], MAX_NAME_LEN + 1) == 0) {
            printf(",\n");
            printf(date_placeholder, 1900 + t->tm_year, t->tm_mon, t->tm_mday, max_score);
        }
        if (i != count[1] - 1)
            printf("\n\t\t]\n\t}, {\n");
    }
    fputs("\n\t\t]\n\t}]\n});\n\t\t</script>\n\t</body>\n</html>\n", stdout); //\t</body>\n</html>\n");

names_cleanup:
    free(names);

changes_cleanup:
    free(changes);

sqlite3_cleanup:

    sqlite3_close(db);
    return res;
}

int show_episode_ownages(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    snprintf(sql_cmd, sizeof sql_cmd, "CREATE TEMP TABLE tmp AS SELECT * FROM scores ORDER BY day DESC LIMIT 6000;");
    rc = sqlite3_exec(db, sql_cmd, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error while creating temporary table:\n[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }

    snprintf(sql_cmd, sizeof sql_cmd,
             "SELECT tmp.level/5, tmp.nick FROM tmp INNER JOIN tmp AS tmp2 WHERE tmp.level<tmp2.level AND tmp.level/5=tmp2.level/5 AND "
             "tmp.nick=tmp2.nick AND tmp.place=0 AND tmp2.place=0 GROUP BY tmp.nick, tmp.level HAVING count(tmp.level)=4;");
    rc = sqlite3_exec(db, sql_cmd, callback_ownage, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error while executing sql query:\n[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    sqlite3_close(db);
    return SQLITE_OK;
}

int generate_all_levels_history(int argc, char **argv, struct tm *t)
{
    (void)argv;
    (void)argc;
    printf("%s", header);
    for (int i = 0; i < 500; ++i) {
        printf("\t\t\t%02d-%d<input type=\"checkbox\" name=\"box\" onclick=\"showGraph(%d)\">", i/5, i%5, i);
        printf("\n");
    }
    for (int i = 0; i < 50; ++i) {
        printf("\t\t\ta%d-%d<input type=\"checkbox\" name=\"box\" onclick=\"showGraph(%d)\">", i/5, i%5, i + 500);
        printf("\n");
    }
    for (int i = 0; i < 50; ++i) {
        printf("\t\t\tb%d-%d<input type=\"checkbox\" name=\"box\" onclick=\"showGraph(%d)\">", i/5, i%5, i + 550);
        if (i == 49) printf("</br>");
        printf("\n");
    }
    printf("\t\t\t<button onclick=\"collapseAll()\">Hide All</button>\n\t\t</form>\n\n\t\t<aside id=\"level-history-note\">\n\t\t\t<h2>Note!!!</h2>\n\t\t\t"
           "<p>This page is comprised of multiple (600) hidden charts, which are displayed below, upon clicking appropriate checkboxes.\n\t\t\t\t"
           "For this reason, loading this page may take one minute, so be patient.</p>\n\t\t\t<p>If you haven't noticed \"<strong>Hide all</strong>\" button at bottom,\n\t\t\t\t"
           "now you are aware of it, and you can use it to hide all charts once you have observed them!</p>\n\t\t</aside>\n\t</main>\n\n");

    for (int lvl = 0; lvl < 600; ++lvl) {
        char level_buffer[6] = {0};
        snprintf(level_buffer, sizeof level_buffer, "%d-%d", lvl /5, lvl %5);

        int rc = generate_level_history(1, (char *[]){level_buffer, NULL}, t);
        if (rc) {
            printf("Error in function: -p %d\n", rc);
            return rc;
        }
    }

    printf("\t</body>\n</html>\n");
    return SQLITE_OK;
}

int generate_diff_level_history(int argc, char **argv, struct tm *t)
{
    (void)argc;

    sqlite3 *db;
    int rc;
    int min_score = -1;
    char *zErrMsg;

    unsigned lvl = str_to_lvl(*argv);

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    snprintf(sql_cmd, sizeof sql_cmd, "SELECT score FROM scores2 WHERE level=%u AND place=9 ORDER BY day ASC LIMIT 1;", lvl);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &min_score, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -2;
    }

    snprintf(form_buffer, sizeof form_buffer, highchart, lvl, "block", lvl, lvl/5, lvl - lvl/5*5, min_score - 5);
    printf("%s", form_buffer);

    if (1) { // 0th places chart
        row_cnt = 0;
        prev_score = -1;
        printf(name_placeholder, "0th score");
        putchar('\n');
        snprintf(sql_cmd, sizeof sql_cmd, 
            "SELECT day, score FROM scores2 WHERE level=%u AND place=%d;", lvl, 0);
        rc = sqlite3_exec(db, sql_cmd, distinc_scores_callback, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return -2;
        }
        printf(date_placeholder, t->tm_year + 1900, t->tm_mon, t->tm_mday, prev_score);
        printf("\n\t\t]\n\t}, {\n");
    }

    if (1) { // 1st places chart
        row_cnt = 0;
        prev_score = -1;
        printf(name_placeholder, "1st score");
        putchar('\n');
        snprintf(sql_cmd, sizeof sql_cmd, 
            "SELECT day, score FROM scores2 WHERE level=%u AND place=%d;", lvl, 1);
        rc = sqlite3_exec(db, sql_cmd, distinc_scores_callback, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return -2;
        }
        printf(date_placeholder, t->tm_year + 1900, t->tm_mon, t->tm_mday, prev_score);
        printf("\n\t\t]\n\t}, {\n");
    }

    if (1) { // 9th places chart
        row_cnt = 0;
        prev_score = -1;
        printf(name_placeholder, "9th score");
        putchar('\n');
        snprintf(sql_cmd, sizeof sql_cmd, 
            "SELECT day, score FROM scores2 WHERE level=%u AND place=%d;", lvl, 9);
        rc = sqlite3_exec(db, sql_cmd, distinc_scores_callback, NULL, &zErrMsg);
        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return -2;
        }
        printf(date_placeholder, t->tm_year + 1900, t->tm_mon, t->tm_mday, prev_score);
        printf("\n\t\t]\n\t}]\n});\n\t\t</script>\n\t</body>\n</html>\n");
    }

    sqlite3_close(db);
    return SQLITE_OK;
}

int update_charts(int argc, char **argv, struct tm *t)
{
    (void)argc;

    sqlite3 *db;
    char *zErrMsg = NULL, *time_string;
    int rc;
    FILE* f_template = NULL;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return -1;
    }

    rc = sqlite3_exec(db, begin_transaction, NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        goto cleanup;
    }

    snprintf(sql_cmd, sizeof sql_cmd, "%s", "SELECT count(DISTINCT day) FROM scores;");
    sqlite3_exec(db, sql_cmd, callback_count, &distinct_days, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        goto cleanup;
    }

    if (f_template = fopen("templates\\RankingsChart-template.htm", "r"), f_template == NULL) {
        goto cleanup;
    }

    if (1) { // Timeline charts!
        unsigned chart_pick_flag = (unsigned)strtoul(*argv, NULL, 0);

#define PICK_0THS_POS       (0U)
#define PICK_PURE_0THS_POS  (1U)
#define PICK_TOP10_POS      (2U)
#define PICK_TOP5_POS       (3U)
#define PICK_PTS_POS        (4U)
#define PICK_AVG_POS        (5U)

        if ((chart_pick_flag >> PICK_0THS_POS) & 1U) {    //0ths

            // ToDo: see if we can refactor this, one function dealing with all timeline charts.
            if (fw = fopen("0thRankingsChart.htm", "w"), fw == NULL) {
                fprintf(stderr, "Error in opening file for writing!\n");
                goto cleanup;
            }

            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
                fputs(buffer, fw);
            }

            for (size_t i = 0; i < NUM_PLAYERS; ++i) {
                if (i != 10)
                    fprintf(fw, "\t\tname: '%s',\n\t\twebsite: 'https://nv2.fandom.com/wiki/%c%s',\n\t\tdata: [\n", players[i], toupper(*players[i]), players[i] + 1);
                else
                    fprintf(fw, "\t\tname: 'White Whale',\n\t\twebsite: 'https://nv2.fandom.com/wiki/White_Whale',\n\t\tdata: [\n");
                row_cnt = 1;
                t->tm_wday = TYPE_INT;
                snprintf(sql_cmd, sizeof sql_cmd, 
                    "SELECT day, count(day) \
                    FROM scores WHERE nick = '%s' AND(place = 0 OR flag_tied = 1) GROUP BY day ORDER BY day ASC;",
                    players[i]);
                sqlite3_exec(db, sql_cmd, callback_oneline, t, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    goto cleanup;
                }
                fprintf(fw, "\t\t]\n\t}");
                if (i == NUM_PLAYERS - 1) fprintf(fw, "]\n})\n\t\t</script>\n\t</body>\n</html>\n");
                else fprintf(fw, ", {\n");
            }
            fclose(fw);
        }
        else
        {
            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
            }
        }

        if ((chart_pick_flag >> PICK_PURE_0THS_POS) & 1U) {   // 0th No Ties
            if (fw = fopen("0thRankingsNoTiesChart.htm", "w"), fw == NULL) {
                fprintf(stderr, "Error in opening file for writing!\n");
                goto cleanup;
            }

            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
                fputs(buffer, fw);
            }

            for (size_t i = 0; i < NUM_PLAYERS; ++i) {
                if (i != 10)
                    fprintf(fw, "\t\tname: '%s',\n\t\twebsite: 'https://nv2.fandom.com/wiki/%c%s',\n\t\tdata: [\n", players[i], toupper(*players[i]), players[i] + 1);
                else
                    fprintf(fw, "\t\tname: 'White Whale',\n\t\twebsite: 'https://nv2.fandom.com/wiki/White_Whale',\n\t\tdata: [\n");
                row_cnt = 1;
                snprintf(sql_cmd, sizeof sql_cmd,
                    "SELECT day, count(day) FROM scores WHERE nick = '%s' AND place = 0 GROUP BY day ORDER BY day ASC;"
                    , players[i]);
                sqlite3_exec(db, sql_cmd, callback_oneline, t, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    goto cleanup;
                }
                fprintf(fw, "\t\t]\n\t}");
                if (i == NUM_PLAYERS - 1) fprintf(fw, "]\n})\n\t\t</script>\n\t</body>\n</html>\n");
                else fprintf(fw, ", {\n");
            }
            fclose(fw);
        }
        else
        {
            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
            }
        }

        if ((chart_pick_flag >> PICK_TOP10_POS) & 1U) {   // Top10s
            if (fw = fopen("Top10RankingsChart.htm", "w"), fw == NULL) {
                fprintf(stderr, "Error in opening file for writing!\n");
                goto cleanup;
            }

            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
                fputs(buffer, fw);
            }

            for (size_t i = 0; i < NUM_PLAYERS; ++i) {
                if (i != 10)
                    fprintf(fw, "\t\tname: '%s',\n\t\twebsite: 'https://nv2.fandom.com/wiki/%c%s',\n\t\tdata: [\n", players[i], toupper(*players[i]), players[i] + 1);
                else
                    fprintf(fw, "\t\tname: 'White Whale',\n\t\twebsite: 'https://nv2.fandom.com/wiki/White_Whale',\n\t\tdata: [\n");
                row_cnt = 1;
                snprintf(sql_cmd, sizeof sql_cmd, 
                    "SELECT day, count(day) FROM scores WHERE nick='%s' GROUP BY day ORDER BY day ASC;",
                    players[i]);
                sqlite3_exec(db, sql_cmd, callback_oneline, t, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    goto cleanup;
                }
                fprintf(fw, "\t\t]\n\t}");
                if (i == NUM_PLAYERS - 1) fprintf(fw, "]\n})\n\t\t</script>\n\t</body>\n</html>\n");
                else fprintf(fw, ", {\n");
            }
            fclose(fw);
        }
        else
        {
            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
            }
        }

        if ((chart_pick_flag >> PICK_TOP5_POS) & 1U) {   // Top5s
            if (fw = fopen("Top5RankingsChart.htm", "w"), fw == NULL) {
                fprintf(stderr, "Error in opening file for writing!\n");
                goto cleanup;
            }

            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
                fputs(buffer, fw);
            }

            for (size_t i = 0; i < NUM_PLAYERS; ++i) {
                if (i != 10)
                    fprintf(fw, "\t\tname: '%s',\n\t\twebsite: 'https://nv2.fandom.com/wiki/%c%s',\n\t\tdata: [\n", players[i], toupper(*players[i]), players[i] + 1);
                else
                    fprintf(fw, "\t\tname: 'White Whale',\n\t\twebsite: 'https://nv2.fandom.com/wiki/White_Whale',\n\t\tdata: [\n");
                row_cnt = 1;
                snprintf(sql_cmd, sizeof sql_cmd, 
                    "SELECT day, count(day) FROM scores WHERE nick='%s' AND place BETWEEN 0 AND 4 GROUP BY day ORDER BY day ASC;",
                    players[i]);
                sqlite3_exec(db, sql_cmd, callback_oneline, t, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    goto cleanup;
                }
                fprintf(fw, "\t\t]\n\t}");
                if (i == NUM_PLAYERS - 1) fprintf(fw, "]\n})\n\t\t</script>\n\t</body>\n</html>\n");
                else fprintf(fw, ", {\n");
            }
            fclose(fw);
        }
        else
        {
            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
            }
        }

        if ((chart_pick_flag >> PICK_PTS_POS) & 1U) {   // Points
            if (fw = fopen("PointsRankingsChart.htm", "w"), fw == NULL) {
                fprintf(stderr, "Error in opening file for writing!\n");
                goto cleanup;
            }

            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
                fputs(buffer, fw);
            }

            for (size_t i = 0; i < NUM_PLAYERS; ++i) {
                if (i != 10)
                    fprintf(fw, "\t\tname: '%s',\n\t\twebsite: 'https://nv2.fandom.com/wiki/%c%s',\n\t\tdata: [\n", players[i], toupper(*players[i]), players[i] + 1);
                else
                    fprintf(fw, "\t\tname: 'White Whale',\n\t\twebsite: 'https://nv2.fandom.com/wiki/White_Whale',\n\t\tdata: [\n");
                row_cnt = 1;
                snprintf(sql_cmd, sizeof sql_cmd, 
                    "SELECT day, 10*count(level) - sum(place) FROM scores WHERE nick='%s' GROUP BY day ORDER BY day ASC;",
                    players[i]);
                sqlite3_exec(db, sql_cmd, callback_oneline, t, &zErrMsg);
                if (rc != SQLITE_OK) {
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    goto cleanup;
                }
                fprintf(fw, "\t\t]\n\t}");
                if (i == NUM_PLAYERS - 1) fprintf(fw, "]\n})\n\t\t</script>\n\t</body>\n</html>\n");
                else fprintf(fw, ", {\n");
            }
            fclose(fw);
        }
        else
        {
            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
            }
        }

        if ((chart_pick_flag >> PICK_AVG_POS) & 1U) {   // Average
            if (fw = fopen("AverageRankingsChart.htm", "w"), fw == NULL) {
                fprintf(stderr, "Error in opening file for writing!\n");
                goto cleanup;
            }

            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
                fputs(buffer, fw);
            }

            for (size_t i = 0; i < NUM_PLAYERS; ++i) {
                if (i != 10)
                    fprintf(fw, "\t\tname: '%s',\n\t\twebsite: 'https://nv2.fandom.com/wiki/%c%s',\n\t\tdata: [\n", players[i], toupper(*players[i]), players[i] + 1);
                else
                    fprintf(fw, "\t\tname: 'White Whale',\n\t\twebsite: 'https://nv2.fandom.com/wiki/White_Whale',\n\t\tdata: [\n");
                row_cnt = 1;
                t->tm_wday = TYPE_FLOAT;
                snprintf(sql_cmd, sizeof sql_cmd, 
                    "SELECT day, avg(place) AS averageRank FROM scores WHERE nick='%s' GROUP BY day",
                    players[i]);
                sqlite3_exec(db, sql_cmd, callback_oneline, t, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    goto cleanup;
                }
                fprintf(fw, "\t\t]\n\t}");
                if (i == NUM_PLAYERS - 1) fprintf(fw, "]\n})\n\t\t</script>\n\t</body>\n</html>\n");
                else fprintf(fw, ", {\n");
            }
            fclose(fw);
        }
        else
        {
            while (fgets(buffer, sizeof buffer, f_template) != NULL) {
                if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
                    break;
            }
        }
    }

    time_string = asctime(t);
    if (time_string)
        time_string[strcspn(time_string, "\n")] = 0;
    else
        time_string = "DOW MON DD HH:MM:SS YYYY";

    if (1) { // Current charts
        if (fw = fopen("current-rankings-charts.htm", "w"), fw == NULL) {
            fprintf(stderr, "Error in opening file for writing!\n");
            goto cleanup;
        }

        fprintf(fw, "%s", curr_header);        

        if (1) { // current 0ths

            snprintf(sql_cmd, sizeof sql_cmd,
                "SELECT sub.nick, count(CASE WHEN sub.flag_tied=1 THEN 1 ELSE NULL END) AS tied, count(CASE WHEN sub.place=0 THEN 1 ELSE NULL END) AS pure FROM ("
                "SELECT * FROM scores ORDER BY day DESC LIMIT 6000) sub "
                "GROUP BY sub.nick HAVING (pure + tied > 0) ORDER BY (pure + tied) DESC, pure DESC"
            );

            struct sql_callback_data *data = &(struct sql_callback_data){ 
                .format = "123", 
                .stats = calloc(100,  sizeof(data->stats[0]))
            };

            if (data->stats == NULL) {
                fprintf(stderr, "WTF\n");
            }
            else 
            {
                rc = sqlite3_exec(db, sql_cmd, callback_stats, data, &zErrMsg);
                if (rc) {
                    fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    fclose(fw);
                    free(data->stats);
                }

                fprintf(fw, "\t<div id=\"container1\" style=\"min-width: 310px; max-width: 1600px; height: 600px; margin: 0 auto\"></div>\n\n\t\t<script type=\"text/javascript\">\n\n");
                fprintf(fw, "Highcharts.chart('container1', {\n\tchart: {\n\t\ttype: 'column',\n\t\tzoomType: 'xy'\n\t},\n\ttitle: {\n\t\ttext: '0ths rankings'\n\t},\n\t");
                fprintf(fw, "subtitle: {\n\t\ttext: 'As of %.3s %d %d'\n\t},\n\txAxis: {\n\t\tcategories: [", time_string + 4, t->tm_mday, t->tm_year + 1900);
                fprintf(fw, "'%s (GM)', ", data->stats[0].nick);
                for (size_t i = 1; i < data->stats_cnt; ++i) {
                    fprintf(fw, "'%s'", data->stats[i].nick);
                    if (i == data->stats_cnt - 1)
                        break;
                    fprintf(fw, ", ");
                }
                fprintf(fw, "]\n\t},\n\tyAxis: {\n\t\tmin: 0,\n\t\tmax: %d,\n\t\ttitle: {\n\t\t\ttext: '0th count'\n\t\t},\n\t\tstackLabels: {\n\t\t\tenabled: true,\n\t\t\tstyle: {\n\t\t\t\t", data->stats[0].pure + data->stats[0].ties + 10);
                fprintf(fw, "fontWeight: 'bold',\n\t\t\t\tcolor: ( //theme\n\t\t\t\t\tHighcharts.defaultOptions.title.style && Highcharts.defaultOptions.title.style.color) || 'gray'\n\t\t\t");
                fprintf(fw, "}\n\t\t}\n\t},\n\tlegend: {\n\t\talign: 'right',\n\t\tx: -30,\n\t\tverticalAlign: 'top',\n\t\ty: 25,\n\t\tbackgroundColor: Highcharts.defaultOptions.legend.backgroundColor || 'white',\n\t\t");
                fprintf(fw, "borderColor: '#CCC',\n\t\tborderWidth: 1,\n\t\tshadow: false\n\t},\n\ttooltip: {\n\t\theaderFormat: '<b>{point.x}</b><br/>',\n\t\tpointFormat: ");
                fprintf(fw, "'{series.name}: {point.y}<br/>Total: {point.stackTotal}'\n\t},\n\tplotOptions: {\n\t\tcolumn: {\n\t\t\tstacking: 'normal',\n\t\t\tdataLabels: {\n\t\t\t\tenabled: false\n\t\t\t}\n\t\t");
                fprintf(fw, "}\n\t},\n\tseries: [{\n");
                fprintf(fw, "\t\tname: 'Tied',\n\t\tdata: [");
                for (size_t i = 0; i < data->stats_cnt; ++i) {
                    fprintf(fw, "%d", data->stats[i].ties);
                    if (i == data->stats_cnt - 1)
                        break;
                    fprintf(fw, ", ");
                }
                fprintf(fw, "]\n\t}, {\n");
                fprintf(fw, "\t\tname: 'Pure',\n\t\tdata: [");
                for (size_t i = 0; i < data->stats_cnt; ++i) {
                    fprintf(fw, "%d", data->stats[i].pure);
                    if (i == data->stats_cnt - 1)
                        break;
                    fprintf(fw, ", ");
                }
                fprintf(fw, "]\n\t}]\n})\n\t\t</script><hr>\n");

                if (1) { // Current pure 0ths
                    qsort(data->stats, data->stats_cnt, sizeof data->stats[0], cmp_pure);

                    fprintf(fw, "\t<div id=\"container2\" style=\"min-width: 310px; max-width: 1200px; height: 600px; margin: 0 auto\"></div>\n\n\t\t<script type=\"text/javascript\">\n\n");
                    fprintf(fw, "Highcharts.chart('container2', {\n\tchart: {\n\t\ttype: 'item'\n\t},\n\ttitle: {\n\t\ttext: 'Pure 0ths rankings'\n\t},\n\tlegend: {\n\t\tlabelFormat: '{name} <span style=\"opacity: 0.4\">{y}</span>'\n\t},");
                    fprintf(fw, "\n\tseries: [{\n\t\tname: '0ths',\n\t\tkeys: ['name', 'y', 'color', 'label'],\n\t\tdata: [\n");
                    for (size_t i = 0; i < data->stats_cnt; ++i) {
                        if (data->stats[i].pure == 0)
                            break;
                        fprintf(fw, "\t\t\t['%s', %3d, '%s'],\n", data->stats[i].nick, data->stats[i].pure, colors[i % 20]);
                    }
                    fprintf(fw, "\t\t],\n\t\tdataLabels: {\n\t\t\tenabled: false,\n\t\t\tformat: '{point.label}'\n\t\t},\n\n\t\t// Circular options\n\t\tcenter: ['50%%', '88%%'],\n\t\t");
                    fprintf(fw, "size: '180%%',\n\t\tstartAngle: -100,\n\t\tendAngle: 100\n\t}]\n});\n\t\t</script><hr>\n");
                }

                free(data->stats);
            }
        }

        if (1) { // Top10s and top5s
            
            snprintf(sql_cmd, sizeof sql_cmd,
                "SELECT sub.nick, count(sub.nick) AS top10s, count(CASE WHEN sub.place BETWEEN 0 AND 4 THEN 1 ELSE NULL END) AS top5s FROM ("
                "SELECT * FROM scores ORDER BY day DESC LIMIT 6000) sub "
                "GROUP BY sub.nick ORDER BY(top10s) DESC, (top5s) DESC LIMIT 20"
            );
            struct sql_callback_data* data = &(struct sql_callback_data) {
                .format = "145", 
                .stats_cnt = 0,
                .stats = (struct stats[20]){ 0 }
            };
            rc = sqlite3_exec(db, sql_cmd, callback_stats, data, &zErrMsg);
            if (rc) {
                fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
                fclose(fw);
            }

            fprintf(fw, "\t<div id=\"container3\" style=\"min-width: 310px; max-width: 1600px; height: 600px; margin: 0 auto\"></div>\n\n\t\t<script type=\"text/javascript\">\n\n");
            fprintf(fw, "Highcharts.chart('container3', {\n\tchart: {\n\t\ttype: 'column',\n\t\tzoomType: 'xy'\n\t},\n\ttitle: {\n\t\ttext: 'Top10s and Top5s rankings'\n\t},\n\txAxis: {\n\t\tcategories: [");
            for (size_t i = 0; i < data->stats_cnt; ++i) {
                fprintf(fw, "'%s'", data->stats[i].nick);
                    if (i == data->stats_cnt - 1)
                        break;
                fprintf(fw, ", ");
            }
            fprintf(fw, "]\n\t},\n\tyAxis: {\n\t\tmin: 0,\n\t\ttitle: {\n\t\t\ttext: 'Top10s and Top5s'\n\t\t},\n\t\tstackLabels: {\n\t\t\tenabled: true,\n\t\t\tstyle: {\n\t\t\t\t");
            fprintf(fw, "fontWeight: 'bold',\n\t\t\t\tcolor: ( //theme\n\t\t\t\t\tHighcharts.defaultOptions.title.style && Highcharts.defaultOptions.title.style.color) || 'gray'\n\t\t\t");
            fprintf(fw, "}\n\t\t}\n\t},\n\tlegend: {\n\t\talign: 'right',\n\t\tx: -30,\n\t\tverticalAlign: 'top',\n\t\ty: 25,\n\t\tbackgroundColor: Highcharts.defaultOptions.legend.backgroundColor || 'white',\n\t\t");
            fprintf(fw, "borderColor: '#CCC',\n\t\tborderWidth: 1,\n\t\tshadow: false\n\t},\n\ttooltip: {\n\t\theaderFormat: '<b>{point.x}</b><br/>',\n\t\tpointFormat: ");
            fprintf(fw, "'{series.name}: {point.y}<br/>Total: {point.stackTotal}'\n\t},\n\tplotOptions: {\n\t\tcolumn: {\n\t\t\tstacking: 'normal',\n\t\t\tdataLabels: {\n\t\t\t\tenabled: false\n\t\t\t}\n\t\t");
            fprintf(fw, "}\n\t},\n\tseries: [{\n");
            fprintf(fw, "\t\tname: 'Non-top5s',\n\t\tdata: [");
            for (size_t i = 0; i < data->stats_cnt; ++i) {
                fprintf(fw, "%d", data->stats[i].top10s - data->stats[i].top5s);
                    if (i == data->stats_cnt - 1)
                        break;
                fprintf(fw, ", ");
            }
            fprintf(fw, "]\n\t}, {\n");
            fprintf(fw, "\t\tname: 'Top5s',\n\t\tdata: [");
            for (size_t i = 0; i < data->stats_cnt; ++i) {
                fprintf(fw, "%d", data->stats[i].top5s);
                    if (i == data->stats_cnt - 1)
                        break;
                fprintf(fw, ", ");
            }
            fprintf(fw, "]\n\t}]\n})\n\t\t</script><hr>\n");
        }

        if (1) { // Points
            
            snprintf(sql_cmd, sizeof sql_cmd,
                "SELECT sub.nick, count(sub.nick) AS top10s, 10 * count(sub.nick) - sum(sub.place) AS pts FROM("
                "SELECT * FROM scores ORDER BY day DESC LIMIT 6000) sub "
                "GROUP BY sub.nick ORDER BY(pts) DESC LIMIT 20"
            );
            struct sql_callback_data* data = &(struct sql_callback_data) {
                .format = "146", 
                .stats_cnt = 0,
                .stats = (struct stats[20]){ 0 }
            };
            rc = sqlite3_exec(db, sql_cmd, callback_stats, data, &zErrMsg);
            if (rc) {
                fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
                fclose(fw);
            }

            fprintf(fw, "\t<div id=\"container4\" style=\"min-width: 310px; max-width: 1400px; height: 800px; margin: 0 auto\"></div>\n\n\t\t<script type=\"text/javascript\">\n\n");
            fprintf(fw, "Highcharts.chart('container4', {\n\tchart: {\n\t\ttype: 'bubble',\n\t\tplotBorderWidth: 1,\n\t\tzoomType: 'xy'\n\t},\n\tlegend: {\n\t\tenabled: false\n\t},\n");
            fprintf(fw, "\ttitle: {\n\t\ttext: 'Points rankings'\n\t},\n\txAxis: {\n\t\tgridLineWidth: 1,\n\t\ttitle: {\n\t\t\ttext: 'Number of Top10 scores'\n\t\t},\n\t\tlabels: {\n");
            fprintf(fw, "\t\t\tformat: '{value}'\n\t\t}\n\t},\n\tyAxis: {\n\t\tstartOnTick: false,\n\t\tendOnTick: false,\n\t\ttitle: {\n\t\t\ttext: 'Average score'\n\t\t},\n\t\tlabels: {\n");
            fprintf(fw, "\t\t\tformat: '{value}'\n\t\t},\n\t\tmaxPadding: 0.2,\n\t\treversed: true\n\t},\n\ttooltip: {\n\t\tuseHTML: true,\n\t\theaderFormat: '<table>',\n\t\tpointFormat: '<tr><th colspan=\"2\"><h3>{point.name}</h3></th></tr>' +");
            fprintf(fw, " '<tr><th>Top10s:</th><td>{point.x}</td></tr>' + '<tr><th>Average score:</th><td>{point.y}</td></tr>' + '<tr><th>Points:</th><td>{point.z}</td></tr>',\n\t\tfooterFormat: '</table>',\n");
            fprintf(fw, "\t\tfollowPointer: true\n\t},\n\tplotOptions: {\n\t\tseries: {\n\t\t\tdataLabels: {\n\t\t\t\tenabled: true,\n\t\t\t\tformat: '{point.name}'\n\t\t\t}\n\t\t}\n\t},\n");
            fprintf(fw, "\tseries: [{\n\t\tdata: [\n");
            for (size_t i = 0; i < data->stats_cnt; ++i)
                fprintf(fw, "\t\t\t{ x: %3d, y: %.6f, z: %4d, name: '%s' },\n", data->stats[i].top10s, 10 - (double)data->stats[i].pts/(data->stats[i].top10s), data->stats[i].pts, data->stats[i].nick);
            fprintf(fw, "]\n\t}]\n});\n\t\t</script>\n");
        }


        fprintf(fw, "\t</body>\n</html>\n");
        
        fclose(fw);
    }

    rc = sqlite3_exec(db, end_transaction, NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

cleanup:
    if (f_template) fclose(f_template);
    sqlite3_close(db);

    return SQLITE_OK;
}

int set_tied_scores(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;
    int score_0th;
    int curr_score;
    char date[11] = "0000-00-00";

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    rc = sqlite3_exec(db, begin_transaction, NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -10;
    }

    snprintf(sql_cmd, sizeof sql_cmd, "%s", "SELECT count(DISTINCT day) FROM scores;");
    sqlite3_exec(db, sql_cmd, callback_count, &distinct_days, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -12;
    }

    for (unsigned int i = 0; i < distinct_days; ++i) {
        snprintf(sql_cmd, sizeof sql_cmd, "SELECT day FROM scores WHERE day>'%s' LIMIT 1;", date);
        sqlite3_exec(db, sql_cmd, callback_day, date, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL Error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return -3;
        }
        for (int j = 0; j < NUM_LEVELS; ++j) {
            snprintf(sql_cmd, sizeof sql_cmd, 
                "SELECT score FROM scores WHERE day='%s' AND level=%d AND place=%d;", date, j, 0);
            sqlite3_exec(db, sql_cmd, callback_count, &score_0th, &zErrMsg);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
                return -4;
            }
            for (int k = 1; k < NUM_ENTRIES; ++k) {
                snprintf(sql_cmd, sizeof sql_cmd, 
                    "SELECT score FROM scores WHERE day='%s' AND level=%d AND place=%d;", date, j, k);
                sqlite3_exec(db, sql_cmd, callback_count, &curr_score, &zErrMsg);
                if (rc != SQLITE_OK) {
                    fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    return -4;
                }
                if (curr_score == score_0th) {
                    snprintf(sql_cmd, sizeof sql_cmd, 
                        "UPDATE scores SET flag_tied=1 WHERE day='%s' AND level=%d AND place=%d;", date, j, k);
                    sqlite3_exec(db, sql_cmd, callback_count, &curr_score, &zErrMsg);
                    if (rc != SQLITE_OK) {
                        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        return -4;
                    }
                }
                else
                    break;
            }
        }
    }

    rc = sqlite3_exec(db, end_transaction, NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -11;
    }

    sqlite3_close(db);
    return SQLITE_OK;
}

int show_older_0ths(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    row_cnt = 0;
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT level/5 as ep, level%%5 as lvl FROM scores2 GROUP BY level HAVING count(CASE WHEN day>='%s' and place=0 THEN 1 ELSE NULL END)=0;", *argv);
    rc = sqlite3_exec(db, sql_cmd, callback_show_level, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    sqlite3_close(db);
    return SQLITE_OK;
}

int generate_histogram(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;
    char player[MAX_NAME_LEN] = {0};

    strcat(player, *argv++);
    while (*argv) {
        player[strlen(player)] = ' ';
        strcat(player, *argv++);
    }

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    generic_int_placeholder = 0; // Keeps maximum value of histogram, which is filled in callback_histogram
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT sub.place, count(sub.place) FROM (SELECT * FROM scores ORDER BY day DESC LIMIT 6000 ) sub WHERE sub.nick='%s' GROUP BY sub.place;", player);
    sqlite3_exec(db, sql_cmd, callback_histogram, histogram, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    print_histogram(player);

    sqlite3_close(db);
    return SQLITE_OK;
}

int generate_changes_list(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -1;
    }

    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT nick, level, place FROM scores WHERE day<='%s' ORDER BY day DESC, level ASC, place ASC LIMIT 6000;", argv[0]);
    sqlite3_exec(db, sql_cmd, callback_fill_leaderboard, oldNames, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    if (argv[1])
        snprintf(sql_cmd, sizeof sql_cmd, "SELECT nick, level, place FROM scores WHERE day<='%s' ORDER BY day DESC, level ASC, place ASC LIMIT 6000;", argv[1]);
    else
        snprintf(sql_cmd, sizeof sql_cmd, "SELECT nick, level, place FROM scores ORDER BY day DESC LIMIT 6000;");

    sqlite3_exec(db, sql_cmd, callback_fill_leaderboard, loadNames, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -3;
    }

    if (argv[1])
        printf("List of Top10 changes between %s and %s.\n\n", argv[0], argv[1]);
    else
        printf("List of Top10 changes since %s.\n\n", argv[0]);

    for (int i = 0; i < NUM_LEVELS; ++i) {
        int j = 0, cnt_new = 0, found = 0, save_j = 0;

        while (j < NUM_ENTRIES && (strncmp(loadNames[i][j], oldNames[i][j], MAX_NAME_LEN) == 0))
            ++j;
        if (j == NUM_ENTRIES) continue;

        save_j = j;

        printf("List of changes for level: %02d-%01d\n-----------------------------------\n", i/5, i%5);
        for ( ; j < NUM_ENTRIES; ++j) {
            int k = 0;
            for ( ; k < NUM_ENTRIES; ++k) {
                if (strncmp(loadNames[i][j], oldNames[i][k], MAX_NAME_LEN) == 0) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                if (j < k) printf("%01d.| %*.*s [+%01d]\n", j, MAX_NAME_LEN, MAX_NAME_LEN, loadNames[i][j], k - j);
                else if (j > k) printf("%01d.| %*.*s [-%01d]\n", j, MAX_NAME_LEN, MAX_NAME_LEN, loadNames[i][j], j - k);
                found = 0;
            }
            else {
                printf("%01d.| %*.*s  [*]\n", j, MAX_NAME_LEN, MAX_NAME_LEN, loadNames[i][j]);
                ++cnt_new;
            }
        }

        for (j = save_j; j < NUM_ENTRIES; j++) {
            found = 0;
            for (int k = save_j; k < NUM_ENTRIES; ++k) {
                if (strncmp(oldNames[i][j], loadNames[i][k], MAX_NAME_LEN) == 0) {
                    found = 1;
                    break;
                }
            }
            if (found == 0) {
                printf("D.| %*.*s  [X]\n", MAX_NAME_LEN, MAX_NAME_LEN, oldNames[i][j]);
                if (--cnt_new == 0)
                    break;
            }
        }
        putchar('\n');
    }

    sqlite3_close(db);

    return SQLITE_OK;
}

int generate_heatmap(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;
    char filename[25] = {0};
    char player[MAX_NAME_LEN] = {0};

    strcat(player, *argv++);
    while (*argv) {
        player[strlen(player)] = ' ';
        strcat(player, *argv++);
    }

    snprintf(filename, sizeof filename, "%.10s-heatmap.htm", player);
    for (int i = 0; i < 10; ++i)
        filename[i] = (char)tolower(filename[i]);
    if (fw = fopen(filename, "w"), fw == NULL) {
        fprintf(stderr, "Error in opening file for writing!\n");
        return -1;
    }

    fprintf(fw, "%s", player_heatmap);
    fprintf(fw, heatmap, player);

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return -2;
    }

    // Read number of rows.
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT count(sub.place) FROM (SELECT nick, place FROM scores ORDER BY day DESC LIMIT 6000 ) sub WHERE sub.nick='%s'", player);
    sqlite3_exec(db, sql_cmd, callback_count, &generic_int_placeholder, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -3;
    }

    row_cnt = 0;
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT sub.level, sub.place FROM (SELECT * FROM scores ORDER BY day DESC LIMIT 6000 ) sub WHERE sub.nick='%s' ORDER BY sub.level", player);
    sqlite3_exec(db, sql_cmd, callback_heatmap, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -3;
    }

    fputs("],\n\t\tdataLabels: {\n\t\t\tenabled: false,\n\t\t\tcolor: '#000000'\n\t\t}\n\t}]\n})\n\t\t</script><hr>\n\n\t"
            "<div id=\"container2\" style=\"min-width: 310px; max-width: 1600px; height: 600px; margin: 0 auto\"></div>\n\n\t\t"
            "<script type=\"text/javascript\">\n\n", fw);
    fputs(histogram_chart, fw);

    row_cnt = 0;
    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT sub.place, count(sub.place) FROM (SELECT * FROM scores ORDER BY day DESC LIMIT 6000 ) sub WHERE sub.nick='%s' GROUP BY sub.place;", player);
    sqlite3_exec(db, sql_cmd, callback_histogram, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -4;
    }

    sqlite3_close(db);

    fprintf(fw, "%hu", histogram[0]);
    for (size_t i = 1; i < HIST_LEN; ++i)
        fprintf(fw, ", %hu", histogram[i]);

    fputs("]\n\t}]\n})\n\t\t</script>\n\t</body>\n</html>\n", fw);
    fclose(fw);

    return SQLITE_OK;
}

int export_level_data(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    unsigned long lvl = strtoul(*argv, NULL, 10);

    if (lvl < 1001)
        return -1;

    // ToDo: Handle when user levels reach ID of at least a million
    if (lvl > 999999U)
        return -2;

    unsigned int level_id = (unsigned) lvl;
    char filename[12], cmd[256];
    snprintf(filename, sizeof filename, "%u.txt", level_id);
    snprintf(cmd, sizeof cmd, download_demos_template, filename);
    IGNORE_RESULT(system(cmd));

    FILE *fr = NULL;
    if (fr = fopen(filename, "r"), fr == NULL) {
        fprintf(stderr, "Error in opening file %s!", filename);
        return -2;
    }

    int c;
    unsigned int encoded_map_len = 0;
    // data is base64 map data
    char encoded_data[16384] = {0};
    while (encoded_map_len < sizeof encoded_data && (c = fgetc(fr)) != EOF)
        encoded_data[encoded_map_len++] = (char)c;

    fclose(fr);

    size_t decoded_len;
    unsigned char decoded_map_data[12288];
    base64_decode(encoded_data, encoded_map_len, decoded_map_data, &decoded_len);

    unsigned char uncompressed_map_data[16384];
    unsigned long uncompressed_len = sizeof uncompressed_map_data;
    int res = uncompress(uncompressed_map_data, &uncompressed_len, decoded_map_data, (uLong)decoded_len);
    if (res != Z_OK) {
        fprintf(stderr, "Error while uncompressing %d!\n", res);
        return -3;
    }

    unsigned int title_len;
    char title[256] = {0};
    title_len = uncompressed_map_data[0] * 16U + uncompressed_map_data[1];
    if (title_len >= sizeof title) {
        fprintf(stderr, "TL;DR %u!\n", title_len);
        return -4;
    }

    putchar('%');
    printf("%.*s", title_len, (char *)uncompressed_map_data + 2);
    putchar('#');

    for (size_t i = title_len + 2; i < uncompressed_len; ++i) {
        putchar("0123456789abcdef"[uncompressed_map_data[i] & 0xF]);
        putchar("0123456789abcdef"[uncompressed_map_data[i] >> 4]);
    }

    putchar('#');
    return 0;
}

static int export_demo_data_inner(unsigned int level_id, int player_id, const char *player_nick, int out_flag) {
    char filename[20];
    char cmd[256];

    snprintf(filename, sizeof filename, "%u-%d.txt", level_id, player_id);
    snprintf(cmd, sizeof cmd, download_demos_template, filename);
    IGNORE_RESULT(system(cmd));

    FILE *fr = NULL;
    if (fr = fopen(filename, "r"), fr == NULL) {
        fprintf(stderr, "Couldn't open file %s!\n", filename);
        return -4;
    }

    int c;
    unsigned int base64_demo_len = 0;
    char base64_demo_data[4096] = {0};
    while (base64_demo_len < sizeof base64_demo_data && (c = fgetc(fr)) != EOF)
        base64_demo_data[base64_demo_len++] = (char)c;

    fclose(fr);

    size_t decoded_len;
    unsigned char decoded_demo_data[3072];
    base64_decode(base64_demo_data, base64_demo_len, decoded_demo_data, &decoded_len);

    unsigned char uncompressed_demo_data[8192];
    unsigned long uncompressed_demo_len = sizeof uncompressed_demo_data;
    int rc = uncompress(uncompressed_demo_data, &uncompressed_demo_len, decoded_demo_data, (uLong)decoded_len);
    if (rc != Z_OK) {
        fprintf(stderr, "Error while uncompressing %d!\n", rc);
        return -5;
    }

    if (out_flag) {
// Comment this for presentable info
//#define OXY_RUDIMENTORY_DEMO
#ifndef OXY_RUDIMENTORY_DEMO
        printf("Demo data for level ");
        if (level_id <= 600) {
            --level_id;
            printf("%02u-%u", level_id / 5, level_id % 5);
        } else
            printf("%u", level_id);
        printf(" and player %s:\n\n", player_nick);

        double t = 90.0;
        char *actions[] = {"N.", "J.", "L.", "LJ.", "R.", "RJ.", "LR.", "LRJ."};
        for (size_t i = 0; i < uncompressed_demo_len; ++i, t -= 0.01666666) {
            if (uncompressed_demo_data[i] < 0x8)
                printf("%5.3f\t%s\n", t, actions[uncompressed_demo_data[i]]);
            else
                printf("X.");
        }

        printf("\n\nDemo length: %lu\n", uncompressed_demo_len - 2);
#else
        char actions[] = {'0','1','2','3','4','5','6','7'};
        for (size_t i = 0; i < uncompressed_demo_len; ++i) {
            if (uncompressed_demo_data[i] < 0x8)
                putchar(actions[uncompressed_demo_data[i]]);
            else
                putchar('X');
        }
#endif
    }

    return (int)(uncompressed_demo_len - 2);
}

int export_demo_data(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    unsigned int level_id = str_to_lvl(*argv++) + 1;

    char *player_nick = *argv;

    int player_id = -1;
    sqlite3 *db;
    char *zErrMsg = NULL;

    int rc = sqlite3_open("highscores.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT player_id FROM players WHERE nick='%s';", player_nick);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &player_id, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    sqlite3_close(db);

    if (player_id < 0) {
        fprintf(stderr, "ID for player [%s] was not found!\n", player_nick);
        return -3;
    }

    export_demo_data_inner(level_id, player_id, player_nick, 1);

    return 0;
}

int produce_compressed_demo(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    unsigned char input_demo[8192];
    unsigned int input_demo_len = 0;
    int c;
    unsigned char code[] = { '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07'};
    while ((c = getchar()) != EOF)
        input_demo[input_demo_len++] = code[c - '0'];

    unsigned long compressed_len = compressBound(input_demo_len);
    unsigned char *compressed_demo = malloc(compressed_len);
    if (compressed_demo == NULL) {
        fprintf(stderr, "Error while allocating memory!\n");
        return -1;
    }

    int res = compress2(compressed_demo, &compressed_len, input_demo, input_demo_len, Z_BEST_COMPRESSION);
    if (res != Z_OK) {
        fprintf(stderr, "Error while compressing data %d!\n", res);
        free(compressed_demo);
        return -2;
    }

    char encoded_demo[4096];
    base64_encode(compressed_demo, compressed_len, encoded_demo, OUTPUT_LEN_IGNORED);
    for (char *x = encoded_demo; *x; ++x)
        putchar(*x);

    free(compressed_demo);
    return 0;
}

int generate_latest_0ths(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;
    FILE *fr;

    if (fr = fopen("index-tmp.htm", "r"), fr == NULL)
        return -1;

    if (fw = fopen("index.htm", "w"), fw == NULL) {
        fclose(fr);
        return -2;
    }

    /* Find the placeholder ("<!--LATEST_0THS-->") for latest 0ths list */
    while (fgets(buffer, sizeof buffer, fr) != NULL) {
        fputs(buffer, fw);
        if (strstr(buffer, "LATEST_0THS"))
            break;
    }

    fclose(fr);

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      fclose(fw);
      return -3;
    }

    sqlite3_exec(db, "SELECT sub.level/5, sub.level%5, sub.nick, max(sub.score - sub2.score), sub2.nick \
        FROM \
        (SELECT day, level, nick, score, place FROM scores2 WHERE day > date('now', '-7 days') AND place=0 ORDER BY day DESC) AS sub \
        INNER JOIN \
        (SELECT * FROM scores ORDER BY day DESC LIMIT 6000) AS sub2 \
        WHERE sub.level=sub2.level AND sub.place=0 AND sub2.place=1 GROUP BY sub.level ORDER BY sub.day DESC;", callback_latest_0ths, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        fclose(fw);
        sqlite3_close(db);
        return -3;
    }

    fputs(index_footer, fw);

    fclose(fw);
    sqlite3_close(db);

    return SQLITE_OK;
}

int get_leaderboard(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    fprintf(stdout, "Level %s leaderboard:\n------------------------------\n", *argv);

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    unsigned lvl = str_to_lvl(*argv);

    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT place, nick, score FROM scores where level=%u order by day desc, place limit 10;", lvl);
    rc = sqlite3_exec(db, sql_cmd, callback_leaderboard, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error while executing sql query: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    putchar('\n');

    sqlite3_close(db);

    return SQLITE_OK;
}

int show_total_score(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    int total_score = 0, total_community_score = 0;
    char *player = *argv;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    int player_id = -1;

    snprintf(sql_cmd, sizeof sql_cmd, 
        "SELECT player_id FROM players WHERE nick='%s';", player);
    rc = sqlite3_exec(db, sql_cmd, callback_count, &player_id, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -2;
    }

    if (player_id < 0) {
        fprintf(stderr, "ID for player [%s] was not found!\n", player);
        sqlite3_close(db);
        return -3;
    }

    rc = sqlite3_exec(db, "CREATE TEMP TABLE tmp AS SELECT * FROM scores ORDER BY day DESC LIMIT 6000;", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -4;
    }

    rc = sqlite3_exec(db, "SELECT sum(score) FROM tmp WHERE place=0;", callback_count, &total_community_score, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -5;
    }

    for (size_t i = 0; i < NUM_LEVELS; ++i) {
        int score = -1;

        snprintf(sql_cmd, sizeof sql_cmd, "SELECT score FROM tmp WHERE level=%zu AND nick='%s';", i, player);
        rc = sqlite3_exec(db, sql_cmd, callback_count, &score, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL Error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            sqlite3_close(db);
            return -6;
        }

        if (score >= 0) {
            total_score += score;
        }
        else {
            int top_score_frame_cnt;
            int frame_cnt;
            int player_0th_id = -1;

            snprintf(sql_cmd, sizeof sql_cmd,
                     "SELECT player_id FROM players INNER JOIN tmp WHERE players.nick=tmp.nick AND tmp.level=%zu AND tmp.place=0;", i);
            rc = sqlite3_exec(db, sql_cmd, callback_count, &player_0th_id, &zErrMsg);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
                sqlite3_close(db);
                return -7;
            }

            if (player_0th_id < 0) {
                fprintf(stderr, "ID for 0th owner on level %02zu-%zu was not found!\n", i/5, i%5);
                sqlite3_close(db);
                return -8;
            }

            top_score_frame_cnt = export_demo_data_inner((unsigned)i + 1U, player_0th_id, "0th owner", 0);
            frame_cnt = export_demo_data_inner((unsigned)i + 1U, player_id, player, 0);

            int score_0th;
            snprintf(sql_cmd, sizeof sql_cmd, 
                "SELECT score FROM tmp WHERE level=%zu AND place=0;", i);
            rc = sqlite3_exec(db, sql_cmd, callback_count, &score_0th, &zErrMsg);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
                sqlite3_close(db);
                return -9;
            }

            // This isn't strictly correct
            // As player for which we try to obtain total score isn't present on leaderboard,
            // we count frames for both 0th owner of the map and for player in question, and
            // then use this difference to produce actual score, despite not knowing if plyer
            // in question collected all gold pieces.
            if (top_score_frame_cnt > 0 && frame_cnt > 0)
                // ToDo: Get the 0th score from DB
                total_score += score_0th - (frame_cnt - top_score_frame_cnt);
        }
    }

    sqlite3_close(db);

    double total_seconds;
    printf("Total community score:\t%d (%f seconds ", total_community_score, total_seconds = total_community_score/ 60.);
    int millis = (int)(total_seconds - (int)(total_seconds)) * 1000;
    printf("- %ld:%02ld:%02ld-%03d)\n", (long)total_seconds/ 3600, ((long)total_seconds% 3600)/ 60, (long)total_seconds% 60, millis);

    printf("Total score for player [%s]:\t%d (%f seconds ", player, total_score, total_seconds = total_score/ 60.);
    millis = (int)(total_seconds - (int)(total_seconds)) * 1000;
    printf("- %ld:%02ld:%02ld-%03d)", (long)total_seconds/ 3600, ((long)total_seconds% 3600)/ 60, (long)total_seconds% 60, millis);

    return 0;
}

int get_maxed_levels(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    rc = sqlite3_exec(db, "SELECT tmp.level/5, tmp.level%5 FROM (SELECT * FROM scores ORDER BY DAY DESC LIMIT 6000) AS tmp GROUP BY tmp.level HAVING count(DISTINCT tmp.score)=1;", callback_show_level, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 2;
    }

    putchar('\n');

    sqlite3_close(db);
    return SQLITE_OK;
}

int get_filled_levels(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)t;

    if (!isdigit(argv[0][0])) {
        fprintf(stderr, "Argument is not a number: [%s]\n", *argv);
        return 1;
    }

    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if ( rc ) {
        fprintf(stderr, "Can't open database: [%s]\n", sqlite3_errmsg(db));
        return 2;
    }

    rc = sqlite3_exec(db, "CREATE TEMP TABLE tmp AS SELECT * FROM scores ORDER BY DAY DESC LIMIT 6000;", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Couldn't create temporary table:\n[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 3;
    }

    snprintf(sql_cmd, sizeof sql_cmd,
             "SELECT tmp2.level/5, tmp2.level%%5 FROM tmp INNER JOIN tmp AS tmp2 "
             "WHERE tmp.place=0 AND tmp.level=tmp2.level AND tmp.score=tmp2.score GROUP BY tmp2.level HAVING count(*)=%c;", argv[0][0]);
    rc = sqlite3_exec(db, sql_cmd, callback_show_level, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Couldn't execute sql query:\n[%s]\nError: [%s]\n", sql_cmd, zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 3;
    }

    putchar('\n');

    sqlite3_close(db);
    return SQLITE_OK;
}

static int show_board_diff_inner(int place1, int place2)
{
    sqlite3 *db;
    char *zErrMsg = NULL;
    int rc;

    rc = sqlite3_open("highscores.db", &db);
    if ( rc ) {
        fprintf(stderr, "Can't open database: [%s]\n", sqlite3_errmsg(db));
        return 1;
    }

    rc = sqlite3_exec(db, "CREATE TEMP TABLE tmp AS SELECT * FROM scores ORDER BY DAY DESC LIMIT 6000;", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Couldn't create temporary table:\n[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 2;
    }

    snprintf(sql_cmd, sizeof sql_cmd,
             "SELECT tmp.level/5, tmp.level%%5, tmp.score-tmp2.score AS diff, tmp.nick FROM tmp INNER JOIN tmp as tmp2 "
             "WHERE tmp.level=tmp2.level AND tmp.place=%d AND tmp2.place=%d ORDER BY diff DESC LIMIT 20;", place1, place2);
    rc = sqlite3_exec(db, sql_cmd, callback_board_diff, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Couldn't execute query: [%s]\n[%s]\n", sql_cmd, zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 3;
    }

    sqlite3_close(db);

    return SQLITE_OK;
}

int show_greatest_board_diff(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    int rc = show_board_diff_inner(0, 9);

    return rc;
}

int generate_activity(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;

    sqlite3* db;
    char* zErrMsg = NULL;
    int rc;
    FILE* f_template;

    rc = sqlite3_open("highscores.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: [%s]\n", sqlite3_errmsg(db));
        return 1;
    }

    if (f_template = fopen("templates\\Activity-template.htm", "r"), f_template == NULL) {
        fprintf(stderr, "Error in opening file template file for reading!\n");
        goto cleanup;
    }

    while (fgets(buffer, sizeof buffer, f_template) != NULL) {
        if (strcmp(buffer, "<!--HIGHCARTS-->\n") == 0)
            break;
        fputs(buffer, stdout);
    }

    row_cnt = 0;
    rc = sqlite3_exec(db, "SELECT day, count(*) FROM scores2 WHERE place=0 AND day!='2013-07-28' GROUP BY day ORDER BY DAY;", activity_callback, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Couldn't create temporary table:\n[%s]\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        fclose(f_template);
        return 2;
    }

    fprintf(stdout, "\t\t]\n\t}]\n})\n\t\t</script>\n\t</body>\n</html>\n");
    fclose(f_template);
cleanup:
    sqlite3_close(db);

    return SQLITE_OK;
}
