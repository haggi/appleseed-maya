#ifndef MTM_LOGGING_H
#define MTM_LOGGING_H

#include <maya/MString.h>
#include <maya/MTimerMessage.h>
#include <maya/MStreamUtils.h>

#define COUT(msg) MStreamUtils::stdOutStream() << msg << "\n"

class Logging
{
public:
    enum LogLevel{
        LevelInfo = 0,
        LevelError = 1,
        LevelWarning = 2,
        LevelProgress = 3,
        LevelDebug = 4,
        LevelNone = 5
    };
    enum OutputType{
        ScriptEditor,
        OutputWindow
    };

    static void setLogLevel( Logging::LogLevel level);
    static void info(MString logString);
    static void warning(MString logString);
    static void error(MString logString);
    static void debug(MString logString);
    static void debugs(MString logString, int level);
    static void progress(MString logString);
    static void detail(MString logString);
};

MString makeSpace(int level);
static  Logging::LogLevel log_level = Logging::LevelInfo;
static  Logging::OutputType log_outtype = Logging::ScriptEditor;

#endif
