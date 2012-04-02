#pragma once


#ifdef OISM_ENABLE_LOG


#include <sstream>
#include <functional>

namespace oism
{

struct Logger
{
    template<class Val> Logger& operator<<(const Val& val)
    {
        ss << val;
        return *this;
    }
    
    operator std::string () const
    {
        return ss.str().c_str();
    }

    Logger() {ss.str("");}
    virtual ~Logger() = 0;

    static std::stringstream ss; // Keep the buffer and clear it in ctor for efficiency

    typedef std::function<void(const std::string&)> Callback;
    static Callback callback;
};

struct ILog : public Logger { ~ILog(); };
struct ELog : public Logger { ~ELog(); };
struct WLog : public Logger { ~WLog(); };

} // namespace oism


#else // OISM_ENABLE_LOG


namespace oism
{

// Dummy for disabled logging
struct Logger
{
    template<class Val> Logger& operator<<(const Val& val) {return *this;}
};
struct ILog : public Logger {};
struct ELog : public Logger {};
struct WLog : public Logger {};

} // namespace oism


#endif // OISM_ENABLE_LOG
