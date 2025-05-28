#ifdef __LOGGER_H__
#define __LOGGER_H__
#include <iostream>
#include <string>

#define LOG_INFO(msg) Logger::Logger::info(msg)
#define LOG_WARN(msg) Logger::Logger::warn(msg)
#define LOG_ERROR(msg) Logger::Logger::error(msg)

namespace Logger {

class Logger {
public:
  static void info(const std::string &message) {
    std::cout << "[INFO] " << message << std::endl;
  }
  static void warn(const std::string &message) {
    std::cout << "[WARN] " << message << std::endl;
  }
  static void error(const std::string &message) {
    std::cout << "[ERROR] " << message << std::endl;
  }
  static void debug(const std::string &message) {
    std::cout << "[DEBUG] " << message << std::endl;
  }
};

} // namespace Logger

#endif // __LOGGER_H__