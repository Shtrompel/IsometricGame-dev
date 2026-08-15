#ifndef GAME_UTILS_LOGGER
#define GAME_UTILS_LOGGER

#include <cassert>
#include <cstdarg>
#include <cstdio>

#include <cstring>
#include <type_traits>
#include <string>

#include <unordered_map>
#include <map>
#include <unordered_set>

#ifdef __FILE_NAME__
#define FILE_NAME __FILE_NAME__
#else
//#define FILE_NAME __FILE__
const static std::string OS_PATH_SLASH = "/";
#define FILESTEM(x)                                                    \
    std::string(std::string_view(x).substr(                            \
                    std::string_view(x).rfind(OS_PATH_SLASH) + 1,      \
                    std::string_view(x).size() -                       \
                        std::string_view(x).rfind(OS_PATH_SLASH) - 1)) \
        .c_str()
#define FILE_NAME FILESTEM(__FILE__)
#endif

#ifdef __func__
#define FUNC_NAME __func__
#else
#define FUNC_NAME __FUNCTION__
#endif

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif // __GNUC__

// Static global variables
// Threading unsafe

namespace Logger
{
inline volatile unsigned priority = 0b1111;

enum LogType : unsigned
{
    Other = 0,
    Error = 1,
    Warning = 2,
    Log = 4,
    Debug = 8
};

static void log(
    const char *file,
    const char *func,
    int line,
    LogType type,
    const char *format,
    ...)
{
    va_list args;
    va_start(args, format);

    FILE *stream = 
        (((type & Error) || (type & Warning)) ? 
        stderr : stdout);
    
    //bool repeated = has_message(msgCounter, {file, line, format});
    
    if (priority & (unsigned)type) // && !repeated)
    {
        const char *typeStr = "";
        switch (type)
        {
        case Other:
            break;
        case Error:
            typeStr = "Error";
            break;
        case Warning:
            typeStr = "Warning";
            break;
        case Log:
            typeStr = "Log";
            break;
        case Debug:
            typeStr = "Debug";
            break;
        }

        if (strcmp(typeStr, ""))
            fprintf(stream, "%s,\t", typeStr);
        if (strcmp(file, ""))
            fprintf(stream, "%s ", file);
        if (line != -1)
            fprintf(stream, "%d", line);
        fprintf(stream, ":\t");
        //fprintf(stream, "%s:\n\tFile: %s, func: %s, line: %d.\n\tMsg:\t", typeStr, file, func, line);
        vfprintf(stream, format, args);
        fprintf(stream, "\n");
    }

    va_end(args);
}

template <
    typename T,
    typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
static void log(
    const char *file,
    const char *func,
    int line,
    LogType type,
    T format,
    ...)
{
    FILE *stream = (type == Error ? stderr : stdout);

    if (priority & (unsigned)type)
    {
        const char *typeStr = nullptr;
        switch (type)
        {
        case Other:
            break;
        case Error:
            typeStr = "Error";
            break;
        case Warning:
            typeStr = "Warning";
            break;
        case Log:
            typeStr = "Log";
            break;
        case Debug:
            typeStr = "Debug";
            break;
        }

        if (strcmp(typeStr, ""))
            fprintf(stream, "%s, ", typeStr);
        if (strcmp(file, ""))
            fprintf(stream, "%s ", file);
        if (line != -1)
            fprintf(stream, "%d: ", line);
        fprintf(stream, "%s", std::to_string(format).c_str());
        fprintf(stream, "\n");
    }
}

static void set_priority(unsigned level)
{
    priority = (2u << level) - 1;
}

static void new_line()
{
    printf("\n");
}
} // namespace Logger

#define LOG(format, ...) \
    Logger::log(FILE_NAME, FUNC_NAME, __LINE__, Logger::LogType::Log, format, ##__VA_ARGS__)

#define DEBUG(format, ...) \
    Logger::log(FILE_NAME, FUNC_NAME, __LINE__, Logger::LogType::Debug, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    Logger::log(FILE_NAME, FUNC_NAME, __LINE__, Logger::LogType::Error, format, ##__VA_ARGS__)

#ifdef WARNING
#undef WARNING
#endif

#define WARNING(format, ...) \
    Logger::log(FILE_NAME, FUNC_NAME, __LINE__, Logger::LogType::Warning, format, ##__VA_ARGS__)

static void assert_error(bool b, const char *ex, const char *msg, const char *file, int line)
{
    if (!b)
        Logger::log(file, "", line, Logger::LogType::Error, "Assertion %s failed: %s", ex, msg);
}

#ifdef NDEBUG
#define ASSERT_ERROR(ex, str) \
    assert_error(ex, #ex, str, __FILE__, __LINE__)
#else
#define ASSERT_ERROR(ex, str) assert(ex &&#str)
#endif

#ifdef __GNUC__

#pragma GCC diagnostic pop

#endif // __GNUC__

#endif // GAME_UTILS_LOGGER
