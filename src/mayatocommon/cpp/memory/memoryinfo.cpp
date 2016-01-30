#include "memoryinfo.h"

#ifdef _WIN32
    #include "windows.h"
    #include "psapi.h"

    static size_t peakUsage = 0;

    size_t getCurrentUsage()
    {
        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        SIZE_T usedMB = pmc.WorkingSetSize / (1024 * 1024);
        if( startUsage == 0)
            startUsage = usedMB;
        if( usedMB > peakUsage)
            peakUsage = usedMB;
        return (usedMB);
    }

    size_t getPeakUsage()
    {
        return (peakUsage);
    }
#else
    size_t getCurrentUsage()
    {
        return 0;
    }

    size_t getPeakUsage()
    {
        return 0;
    }
#endif
