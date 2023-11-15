#include <stdio.h>

#include "osx_time.h"

struct mach_timebase_info mti;

void initTimebaseInfo(void)
{
  kern_return_t result;
  if ((result = mach_timebase_info(&mti)) != KERN_SUCCESS) fprintf(stderr, "Failed to initialize timebase info. Error code: %d", result);
}

