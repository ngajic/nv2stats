#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "util.h"

unsigned str_to_lvl(const char *str)
{
    assert(str);
    unsigned lvl = (unsigned)-1;
    char *p = NULL;

    if (isalpha(*str)) {
        assert(isdigit(str[1]));
        lvl = 5 * (unsigned)(((*str == 'a' ? 100 : 110) + str[1] - '0'));
    }
    else lvl = 5 * (unsigned)strtoul(str, NULL, 10);

    p = strchr(str, '-');
    if (p)
    {
        assert(isdigit(p[1]) && p[1] < '5');
        lvl += (unsigned)(p[1] - '0');
    }

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
