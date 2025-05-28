#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>

class logger {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    static void log(const std::string& message, Level level = Level::INFO) {
        std::cout << "[" << currentDateTime() << "] "
                  << "[" << levelToString(level) << "] "
                  << message << std::endl;
    }

private:
    static std::string levelToString(Level level) {
        switch (level) {
            case Level::INFO:    return "INFO";
            case Level::WARNING: return "WARNING";
            case Level::ERROR:   return "ERROR";
            case Level::DEBUG:   return "DEBUG";
            default:             return "UNKNOWN";
        }
    }

    static std::string currentDateTime() {
        std::time_t now = std::time(nullptr);
        std::tm* tmPtr = std::localtime(&now);

        std::ostringstream oss;
        oss << std::put_time(tmPtr, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

#endif
