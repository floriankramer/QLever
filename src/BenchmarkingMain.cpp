// Copyright 2014, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include <stdlib.h>
#include <getopt.h>
#include <string>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <libgen.h>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "util/ReadableNumberFact.h"
#include "parser/SparqlParser.h"
#include "util/Timer.h"
#include "engine/QueryPlanner.h"

using std::string;
using std::cout;
using std::endl;
using std::flush;
using std::cerr;

#define EMPH_ON  "\033[1m"
#define EMPH_OFF "\033[22m"

// Available options.
struct option options[] = {
{"all-permutations",  no_argument,       NULL, 'a'},
{"cost-factors",      required_argument, NULL, 'c'},
{"index",             required_argument, NULL, 'i'},
{"log" ,              no_argument,       NULL, 'l'},
{"on-disk-literals",  no_argument,       NULL, 'o'},
{"runs",              required_argument, NULL, 'r'},
{"text",              no_argument,       NULL, 't'},
{NULL,                0,                 NULL, 0}
};

struct QueryResult {
  double timeMs = 0;
  size_t numResults = 0;
};

QueryResult processQuery(QueryExecutionContext& qec, const string& query);
void printHelp(char *execName);

void printHelp(char *execName) {
  printf("Usage: %s [Options] file1, file2, ...\n", execName);
  printf("Runs the queries in the given files writing log data to a file.\n\n");
  printf("Options:\n");
  printf(" %-20s    %s\n", "a, all-permutations", "Use all permutations of the"
                                                 "index.");
  printf(" %-20s    %s\n", "c, cost-factors", "Set the cost factors from a "
                                             "file.");
  printf(" %-20s    %s\n", "i, index", "The indexes file path.");
  printf(" %-20s    %s\n", "l, log", "The log file path.");
  printf(" %-20s    %s\n", "o, on-disk-literals", "Store literals on disk.");
  printf(" %-20s    %s\n", "r, runs", "How often to run every query. The "
                                     "resulting runtime is the average "
                                     "over all runs.");
  printf(" %-20s    %s\n", "t, text", "If the index contains textual data.");
}


// Main function.
int main(int argc, char** argv) {
  cout.sync_with_stdio(false);
  char* locale = setlocale(LC_CTYPE, "en_US.utf8");

  string queryfile;
  string indexName = "";
  string costFactorsFileName = "";
  bool text = false;
  bool onDiskLiterals = false;
  bool allPermutations = false;
  size_t numRuns = 5;
  string logfile = "benchmark.log";
  std::vector<string> queryFiles;

  optind = 1;
  // Process command line arguments.
  while (true) {
    int c = getopt_long(argc, argv, "ac:i:l:or:t", options, NULL);
    if (c == -1) break;
    switch (c) {
      case 'a':
        allPermutations = true;
        break;
      case 'c':
        costFactorsFileName = optarg;
        break;
      case 'i':
        indexName = optarg;
        break;
      case 'l':
        logfile = optarg;
        break;
      case 'o':
        onDiskLiterals = true;
        break;
      case 'r':
        numRuns = atoi(optarg);
        break;
      case 't':
        text = true;
        break;
      default:
        cerr << endl
             << "! ERROR in processing options (getopt returned '" << c
             << "' = 0x" << std::setbase(16) << c << ")"
             << endl << endl;
        exit(1);
    }
  }

  if (argc - optind < 1) {
    cerr << "No input files specified..." << endl;
    printHelp(argv[0]);
    exit(1);
  }

  if (indexName.size() == 0) {
    cerr << "Missing required argument --index (-i)..." << endl;
    printHelp(argv[0]);
    exit(1);
  }

  for (int i = optind; i < argc; i++) {
    queryFiles.emplace_back(argv[i]);
  }

  std::cout << std::endl << EMPH_ON
            << "SparqlEngineMain, version " << __DATE__
            << " " << __TIME__ << EMPH_OFF << std::endl << std::endl;
  cout << "Set locale LC_CTYPE to: " << locale << endl;

  std::ofstream log;
  log.open(logfile);
  if (!log.is_open()) {
    cerr << "Unable to open log file " << logfile << endl;
    exit(1);
  }

  try {
    Engine engine;
    Index index;
    index.createFromOnDiskIndex(indexName, allPermutations, onDiskLiterals);
    if (text) {
      index.addTextFromOnDiskIndex();
    }

    QueryExecutionContext qec(index, engine);
    if (costFactorsFileName.size() > 0) {
      qec.readCostFactorsFromTSVFile(costFactorsFileName);
    }

    for (string queryfile : queryFiles) {
      std::ifstream qf(queryfile);
      std::stringstream queryBuf;
      queryBuf << qf.rdbuf();
      std::string query = queryBuf.str();
      QueryResult result;
      vector<double> times;
      for (size_t numRun = 0; numRun < numRuns; numRun++) {
        QueryResult runResult = processQuery(qec, query);
        result.timeMs += runResult.timeMs;
        result.numResults = runResult.numResults;
        times.push_back(runResult.timeMs);
      }
      double averageTime = result.timeMs / numRuns;
      double sd = 0;
      for (double d : times) {
        sd += std::pow(d - averageTime, 2);
      }
      sd = std::sqrt(sd / (times.size()));
      log << "Query:\n" << query << endl;
      log << "Num results: " << result.numResults << endl;
      log << "Average time: " << averageTime << " ms" << endl;
      log << "All times: [";
      for (size_t i = 0; i < times.size(); i++) {
        log << times[i];
        if (i + 1 < times.size()) {
          log << ", ";
        }
      }
      log << "]" << endl;
      log << "Standard derivation: " << sd << endl;
      log << endl << endl;
    }
  } catch (const std::exception& e) {
    cerr << string("Caught exceptions: ") + e.what() << std::endl;
    return 1;
  } catch (ad_semsearch::Exception& e) {
    cerr << e.getFullErrorMessage() << std::endl;
  }
  return 0;
}

QueryResult processQuery(QueryExecutionContext& qec, const string& query) {
  qec.clearCache();
  QueryResult result;
  ad_utility::Timer t;
  t.start();
  SparqlParser sp;
  ParsedQuery pq = sp.parse(query);
  pq.expandPrefixes();
  QueryPlanner qp(&qec);
  ad_utility::Timer timer;
  timer.start();
  auto qet = qp.createExecutionTree(pq);
  timer.stop();
  size_t limit = MAX_NOF_ROWS_IN_RESULT;
  size_t offset = 0;
  if (pq._limit.size() > 0) {
    limit = static_cast<size_t>(atol(pq._limit.c_str()));
  }
  if (pq._offset.size() > 0) {
    offset = static_cast<size_t>(atol(pq._offset.c_str()));
  }
  // qet.writeResultToStream(cout, pq._selectedVariables, limit, offset);
  t.stop();
  result.timeMs = t.usecs() / 1000.0;
  result.numResults = qet.getResult().size();
  std::cout << "\nDone. Time: " << t.usecs() / 1000.0 << " ms\n";
  std::cout << "\nNumber of matches (no limit): " << qet.getResult().size() <<
               "\n";
  size_t effectiveLimit = atoi(pq._limit.c_str()) > 0 ?
                            atoi(pq._limit.c_str()) : qet.getResult().size();
  std::cout << "\nNumber of matches (limit): " <<
               std::min(qet.getResult().size(), effectiveLimit) << "\n";
  return result;
}
