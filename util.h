#ifndef OXY_UTIL_H
#define OXY_UTIL_H

#define OXY_NV2STATS_VERSION	"1.0.0"
#define OXY_NV2STATS_VERSION_NUMBER 100000

#define LINEBREAK   "=========================================================\n"

/** ************************************************************************************************
 *  \brief        str_to_lvl
 *                Returns level number suitable for use as level ID in highscores.db database. For 
 *                the purposes of usage as level identifier on metanet server side, increase result 
 *                by one.
 *
 *  \param [in]   const char * str
 *
 *  \return [out] Level ID
 */
unsigned str_to_lvl(const char *str);
int get_fw_version(int, char **, struct tm *);

#define CONCAT(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#endif // OXY_UTIL_H
