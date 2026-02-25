#include <time.h>
#include "opt.h"

int main(int argc, char **argv)
{
    opt_callback_func *cb;

    optget(&argc, &argv, &cb);

    if (cb) return cb(argc, argv, localtime(&(time_t) { time(NULL) }));
}
