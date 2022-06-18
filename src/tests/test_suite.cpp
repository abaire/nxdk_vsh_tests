#include "test_suite.h"

#include <fstream>

#include "SDL_stdinc.h"
#include "SDL_test_fuzzer.h"
#include "debug_output.h"
#include "logger.h"
#include "pbkit_ext.h"
#include "test_host.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

TestSuite::TestSuite(TestHost& host, std::string output_dir, std::string suite_name)
    : host_(host), output_dir_(std::move(output_dir)), suite_name_(std::move(suite_name)) {
  output_dir_ += "\\";
  output_dir_ += suite_name_;
  std::replace(output_dir_.begin(), output_dir_.end(), ' ', '_');
}

std::vector<std::string> TestSuite::TestNames() const {
  std::vector<std::string> ret;
  ret.reserve(tests_.size());

  for (auto& kv : tests_) {
    ret.push_back(kv.first);
  }
  return std::move(ret);
}

void TestSuite::DisableTests(const std::vector<std::string>& tests_to_skip) {
  for (auto& name : tests_to_skip) {
    tests_.erase(name);
  }
}

void TestSuite::Run(const std::string& test_name) {
  auto it = tests_.find(test_name);
  if (it == tests_.end()) {
    ASSERT(!"Invalid test name");
  }

  auto start_time = LogTestStart(test_name);
  it->second();
  LogTestEnd(test_name, start_time);
}

void TestSuite::RunAll() {
  auto names = TestNames();
  for (const auto& test_name : names) {
    Run(test_name);
  }
}

void TestSuite::Initialize() { host_.ClearState(); }

std::chrono::steady_clock::time_point TestSuite::LogTestStart(const std::string& test_name) {
  PrintMsg("Starting %s::%s\n", suite_name_.c_str(), test_name.c_str());

#ifdef ENABLE_PROGRESS_LOG
  if (allow_saving_) {
    Logger::Log() << "Starting " << suite_name_ << "::" << test_name << std::endl;
  }
#endif

  return std::chrono::high_resolution_clock::now();
}

void TestSuite::LogTestEnd(const std::string& test_name, std::chrono::steady_clock::time_point start_time) const {
  auto now = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

  PrintMsg("  Completed %s %lums\n", test_name.c_str(), elapsed);

#ifdef ENABLE_PROGRESS_LOG
  if (allow_saving_) {
    Logger::Log() << "  Completed " << test_name << " " << elapsed << "ms" << std::endl;
  }
#endif
}

float TestSuite::RandomFloat() {
  uint32_t val = SDLTest_RandomUint32();
  return *(float*)&val;
}

void TestSuite::RandomVector(VECTOR out) {
  out[0] = RandomFloat();
  out[1] = RandomFloat();
  out[2] = RandomFloat();
  out[3] = RandomFloat();
}
