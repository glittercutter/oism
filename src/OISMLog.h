#pragma once

#include <iostream>
#include <fstream>

#ifdef OISM_USE_LOG

#ifdef OISM_USE_LOG_FILE
static std::ofstream LOG_FILE;
#define LOG_WRITE(_STR) LOG_FILE << _STR; std::cout << _STR;
#else
#define LOG_WRITE(_STR) std::cout << _STR;
#endif

#define LOG_IMP(_LINE) \
{ \
    std::stringstream ss; \
    ss << "oism -- " << _LINE << std::endl; \
    LOG_WRITE(ss.str()); \
}

#define LOG(_LINE) LOG_IMP("LOG: " << _LINE)
#define WLOG(_LINE) LOG_IMP("WARNING: " << _LINE)
#define ELOG(_LINE) LOG_IMP("ERROR: " << _LINE << " -- " __FILE__ << " " << __FUNCTION__ << " " << __LINE__)

#else // OISM_USE_LOG

#define LOG(_LINE) ;
#define WLOG(_LINE) ;
#define ELOG(_LINE) ;

#endif // OISM_USE_LOG
