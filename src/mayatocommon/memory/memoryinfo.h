//
// memory info - for windows only at the moment
//

#ifndef MEMINFO_H
#define MEMINFO_H

#include <stddef.h>

static size_t startUsage = 0;

size_t getCurrentUsage();
size_t getPeakUsage();


#endif
