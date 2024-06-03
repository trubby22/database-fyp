#include <BOSS.hpp>
#include <ExpressionUtilities.hpp>

#include "config.hpp"
#include "dataGeneration.cpp"
#include "group.cpp"
#include "partition.cpp"
#include "select.cpp"
#include "tpch.cpp"
#include "utilities.cpp"
#include <benchmark/benchmark.h>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace boss::utilities;

VTuneAPIInterface vtune{"BOSS"};

bool USING_COORDINATOR_ENGINE = false;
bool VERBOSE_QUERY_OUTPUT = false;
bool VERY_VERBOSE_QUERY_OUTPUT = false;
bool VERIFY_QUERY_OUTPUT = false;
bool ENABLE_CONSTRAINTS = false;
int BENCHMARK_MIN_WARMPUP_ITERATIONS = 3;
int BENCHMARK_MIN_WARMPUP_TIME = 1;
uint64_t SPAN_SIZE_FOR_NON_ARROW = 1U << 23U;
bool BENCHMARK_STORAGE_BLOCK_SIZE = false;
int64_t DEFAULT_STORAGE_BLOCK_SIZE = 0; // 0: keep storage's default
int VELOX_INTERNAL_BATCH_SIZE = 0;
int VELOX_MINIMUM_OUTPUT_BATCH_SIZE = 0;

std::vector<std::string> librariesToTest = {};
std::string storageLibrary = {};
int latestDataSize = -1;
int latestBlockSize = -1;
std::string latestDataSet;

int googleBenchmarkApplyParameterHelper = -1;

static void releaseBOSSEngines() {
  // make sure to release engines in reverse order of evaluation
  // (important for data ownership across engines)
  auto reversedLibraries = librariesToTest;
  std::reverse(reversedLibraries.begin(), reversedLibraries.end());
  boss::expressions::ExpressionSpanArguments spans;
  spans.emplace_back(boss::expressions::Span<std::string>(reversedLibraries));
  boss::evaluate("ReleaseEngines"_(boss::ComplexExpression("List"_, {}, {}, std::move(spans))));
}

void initStorageEngine_bixi(int dataSize, int blockSize) {
  resetStorageEngine();

  auto evalStorage = getEvaluateStorageLambda();
  auto checkForErrors = getCheckForErrorsLambda();

  if(blockSize > 0) {
    checkForErrors(evalStorage("Set"_("FileLoadingBlockSize"_, blockSize)));
  }

  checkForErrors(evalStorage("CreateTable"_("BIXI"_, "duration_sec"_, "latitude_x"_, "longitude_x"_, "latitude_y"_, "longitude_y"_)));

  auto filenamesAndTables = std::vector<std::pair<std::string, boss::Symbol>>{
      {"bixi-clean", "BIXI"_}};

  for(auto const& [filename, table] : filenamesAndTables) {
    std::string path =
        tpch_filePath_prefix + "bixi-data/" + filename + ".tbl";
    checkForErrors(evalStorage("Load"_(table, path)));
  }
}

void initAndRunBenchmarks(int argc, char** argv) {
  storageLibrary = USING_COORDINATOR_ENGINE ? librariesToTest[1] : librariesToTest[0];

  releaseBOSSEngines();
}

int main(int argc, char** argv) {
  try {
    initAndRunBenchmarks(argc, argv);
  } catch(std::exception& e) {
    std::cerr << "caught exception in main: " << e.what() << std::endl;
    boss::evaluate("ResetEngines"_());
    return EXIT_FAILURE;
  } catch(...) {
    std::cerr << "unhandled exception." << std::endl;
    boss::evaluate("ResetEngines"_());
    return EXIT_FAILURE;
  }
  boss::evaluate("ResetEngines"_());
  return EXIT_SUCCESS;
}
