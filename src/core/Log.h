#pragma once

#include <format>
#include <string>
#include <utility>

namespace neta::core {

// Lightweight leveled logger. Everything goes to stderr so game output and
// logs never mix on stdout (stdout is reserved for headless sim output).
enum class LogLevel { Debug, Info, Warning, Error };

class Log {
public:
    static void init(bool verboseOutput);
    static bool verbose();

    template <typename... Args>
    static void debug(const std::string& tag, std::format_string<Args...> fmt, Args&&... args) {
        write(LogLevel::Debug, tag, std::format(fmt, std::forward<Args>(args)...));
    }
    template <typename... Args>
    static void info(const std::string& tag, std::format_string<Args...> fmt, Args&&... args) {
        write(LogLevel::Info, tag, std::format(fmt, std::forward<Args>(args)...));
    }
    template <typename... Args>
    static void warning(const std::string& tag, std::format_string<Args...> fmt, Args&&... args) {
        write(LogLevel::Warning, tag, std::format(fmt, std::forward<Args>(args)...));
    }
    template <typename... Args>
    static void error(const std::string& tag, std::format_string<Args...> fmt, Args&&... args) {
        write(LogLevel::Error, tag, std::format(fmt, std::forward<Args>(args)...));
    }

    // Raw-string overloads for messages that are already formatted.
    static void debugText(const std::string& tag, const std::string& message);
    static void infoText(const std::string& tag, const std::string& message);
    static void warningText(const std::string& tag, const std::string& message);
    static void errorText(const std::string& tag, const std::string& message);

private:
    static void write(LogLevel level, const std::string& tag, const std::string& message);
    static bool s_verbose;
};

}  // namespace neta::core
