#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include <yaml-cpp/yaml.h>

void printHelp(char* name) {
  std::cout << "Usage: " << name << " <input-file> <output-file>" << std::endl;
  std::cout
      << "Parses the input yaml file and writes all queries within into the "
         "output file if they can be parsed and contain a property path"
      << std::endl;
}

struct Query {
  std::string name;
  std::string sparql;
  std::string type;
};

struct QuerySet {
  std::string kb;
  std::vector<Query> queries;
};

QuerySet parseQuerySet(const std::string& path) {
  QuerySet set;
  YAML::Node in = YAML::LoadFile(path);
  set.kb = in["kb"].as<std::string>();
  const YAML::Node& queries = in["queries"];
  for (const YAML::Node& query : queries) {
    Query q;
    q.name = query["query"].as<std::string>();
    q.sparql = query["sparql"].as<std::string>();
    q.type = query["type"].as<std::string>();
    set.queries.emplace_back(q);
  }
  return set;
}

void writeQuerySet(const std::string& path, const QuerySet& set) {
  YAML::Node out;
  out["kb"] = set.kb;
  for (const Query& q : set.queries) {
    YAML::Node query;
    query["query"] = q.name;
    query["sparql"] = q.sparql;
    query["type"] = q.type;
    out["queries"].push_back(query);
  }
  std::ofstream f(path);
  f << out;
}

bool doesQueryContainPropertyPath(const Query& q) { return true; }

int main(int argc, char** argv) {
  if (argc != 3) {
    printHelp(argv[0]);
    return 1;
  }
  QuerySet in = parseQuerySet(std::string(argv[1]));
  QuerySet out;
  out.kb = in.kb;
  for (const Query& q : in.queries) {
    if (doesQueryContainPropertyPath(q)) {
      out.queries.emplace_back(q);
    }
  }
  writeQuerySet(std::string(argv[2]), out);
  return 0;
}
