#ifndef OXY_UTIL_H
#define OXY_UTIL_H

#define OXY_NV2STATS_VERSION	"1.0.0"
#define OXY_NV2STATS_VERSION_NUMBER 100000

#define LINEBREAK   "=========================================================\n"

int str_to_lvl(const char *);
int get_fw_version(int, char **, struct tm *);

#define CONCAT(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#endif // OXY_UTIL_H
