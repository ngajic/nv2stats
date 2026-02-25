#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "util.h"

int str_to_lvl(const char *str)
{
    int lvl = -1;
    char *p = NULL;

    if (isalpha(*str)) {
        lvl = 5 * ((*str == 'a' ? 100 : 110) + str[1] - '0');
    }
    else lvl = 5 * strtol(str, NULL, 10);

    p = strchr(str, '-');
    if (p)
        lvl += p[1] - '0';

    return lvl;
}

int get_fw_version(int argc, char **argv, struct tm *t)
{
    (void)argc;
    (void)argv;
    (void)t;
	puts("" OXY_NV2STATS_VERSION "");
	return 0;
}
