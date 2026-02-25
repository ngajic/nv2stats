#ifndef OXY_OPT_H
#define OXY_OPT_H

typedef int opt_callback_func(int, char **, struct tm *);

void optget(int *, char ***, opt_callback_func **);

#endif // OXY_OPT_H
