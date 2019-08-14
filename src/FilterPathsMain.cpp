#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "parser/SparqlParser.h"

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

bool doesQueryContainPropertyPath(const Query& q) {
  try {
    ParsedQuery pq = SparqlParser(q.sparql).parse();
    pq.expandPrefixes();
    std::vector<std::shared_ptr<const ParsedQuery::GraphPattern>>
        patterns_to_process;
    patterns_to_process.emplace_back(pq._rootGraphPattern);
    while (!patterns_to_process.empty()) {
      std::shared_ptr<const ParsedQuery::GraphPattern> p =
          patterns_to_process.back();
      patterns_to_process.pop_back();
      for (const SparqlTriple& t : p->_whereClauseTriples) {
        if (t._p._operation != PropertyPath::Operation::IRI) {
          return true;
        }
      }

      for (const std::shared_ptr<const ParsedQuery::GraphPatternOperation>& o :
           p->_children) {
        switch (o->_type) {
          case ParsedQuery::GraphPatternOperation::Type::OPTIONAL:
          case ParsedQuery::GraphPatternOperation::Type::UNION:
            patterns_to_process.insert(patterns_to_process.begin(),
                                       o->_childGraphPatterns.begin(),
                                       o->_childGraphPatterns.end());
            break;
          case ParsedQuery::GraphPatternOperation::Type::SUBQUERY:
            patterns_to_process.push_back(o->_subquery->_rootGraphPattern);
            break;
          case ParsedQuery::GraphPatternOperation::Type::TRANS_PATH:
            return true;
            break;
        }
      }
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return false;
  } catch (...) {
    return false;
  }
  return false;
}

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
