#include "OISMLog.h"

#ifdef OISM_ENABLE_LOG

using namespace oism;

// Static member
std::stringstream Logger::ss;
Logger::Callback Logger::callback;

// Pure virtual destructor need an implementation
inline Logger::~Logger() {}

ILog::~ILog()
{
    if (callback) callback(ss.str());
}

ELog::~ELog()
{
    if (callback) callback("==ERROR== " + ss.str());
}

WLog::~WLog()
{
    if (callback) callback("==WARNING== " + ss.str());
}

#endif // OISM_ENABLE_LOG
