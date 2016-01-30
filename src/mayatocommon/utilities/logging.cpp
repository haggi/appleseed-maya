#include "threads/queue.h"
#include "utilities/logging.h"
#include "memory/memoryinfo.h"
#include <maya/MGlobal.h>
#include <stdio.h>

void Logging::setLogLevel( Logging::LogLevel level)
{
    if (level == Logging::LevelDebug)
    {
        MGlobal::displayInfo("Set logging level to DEBUG");
    }
    if (level == Logging::LevelInfo)
    {
        MGlobal::displayInfo("Set logging level to INFO");
    }
    if (level == Logging::LevelWarning)
    {
        MGlobal::displayInfo("Set logging level to WARNING");
    }
    if (level == Logging::LevelError)
    {
        MGlobal::displayInfo("Set logging level to ERROR");
    }
    if (level == Logging::LevelProgress)
    {
        MGlobal::displayInfo("Set logging level to PROGRESS");
    }
    log_level = level;
}

void Logging::info(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelInfo)
        COUT(outString);
}

void Logging::warning(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelWarning)
        COUT(outString);
}

void Logging::error(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelError)
    {
        COUT(outString);
    }
}

void Logging::debug(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelDebug)
    {
        COUT(outString);
    }
}

void Logging::debugs(MString logString, int level)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " + makeSpace(level) + logString;
    if (log_level >= Logging::LevelDebug)
    {
        COUT(outString);
    }
}

void Logging::progress(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelProgress)
    {
        COUT(outString);
    }
}

void Logging::detail(MString logString)
{
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    COUT(outString);
}

MString makeSpace(int level)
{
    MString space;
    for( int i = 0; i < level; i++)
        space += "\t";
    return space;
}
