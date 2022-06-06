#ifndef NXDK_PGRAPH_TESTS_LOGGER_H
#define NXDK_PGRAPH_TESTS_LOGGER_H

#include <fstream>
#include <string>

class Logger {
 public:
  static void Initialize(const std::string &log_path, bool truncate_log);

  static std::ofstream &Log();

 private:
  Logger(const std::string &path, bool truncate_log = false);

  std::ofstream log_file_;

  static Logger *singleton_;
};

#endif  // NXDK_PGRAPH_TESTS_LOGGER_H
