#include "core/Log.h"

#include <cstdio>
#include <ctime>

namespace neta::core {

bool Log::s_verbose = false;

void Log::init(bool verboseOutput) {
    s_verbose = verboseOutput;
}

bool Log::verbose() {
    return s_verbose;
}

void Log::debugText(const std::string& tag, const std::string& message) {
    write(LogLevel::Debug, tag, message);
}
void Log::infoText(const std::string& tag, const std::string& message) {
    write(LogLevel::Info, tag, message);
}
void Log::warningText(const std::string& tag, const std::string& message) {
    write(LogLevel::Warning, tag, message);
}
void Log::errorText(const std::string& tag, const std::string& message) {
    write(LogLevel::Error, tag, message);
}

void Log::write(LogLevel level, const std::string& tag, const std::string& message) {
    if (level == LogLevel::Debug && !s_verbose) {
        return;
    }
    const char* levelName = "INFO";
    switch (level) {
        case LogLevel::Debug: levelName = "DEBUG"; break;
        case LogLevel::Info: levelName = "INFO"; break;
        case LogLevel::Warning: levelName = "WARN"; break;
        case LogLevel::Error: levelName = "ERROR"; break;
    }

    std::time_t now = std::time(nullptr);
    char stamp[16] = "??:??:??";
#if defined(_WIN32)
    std::tm tmBuf{};
    localtime_s(&tmBuf, &now);
    std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &tmBuf);
#else
    std::tm* tmPtr = std::localtime(&now);
    if (tmPtr != nullptr) {
        std::strftime(stamp, sizeof(stamp), "%H:%M:%S", tmPtr);
    }
#endif
    std::fprintf(stderr, "[%s] [%-5s] [%-6s] %s\n", stamp, levelName, tag.c_str(), message.c_str());
    std::fflush(stderr);
}

}  // namespace neta::core
