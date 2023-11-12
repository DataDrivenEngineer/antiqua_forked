#ifndef OSX_TIME_H
#define OSX_TIME_H

#include <mach/mach_time.h>

#include "types.h"

#define TIMESTAMP_TO_NS(ts) (ts) * mti.numer / mti.denom

extern struct mach_timebase_info mti;

MONExternC void initTimebaseInfo(void);

#endif
