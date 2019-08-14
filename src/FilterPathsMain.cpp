#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include "parser/SparqlParser.h"

const std::string prefixes = R"(
PREFIX p: <http://www.wikidata.org/prop/>
PREFIX psn: <http://www.wikidata.org/prop/statement/value-normalized/>
PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
PREFIX wd: <http://www.wikidata.org/entity/>
PREFIX wdt: <http://www.wikidata.org/prop/direct/>
PREFIX wikibase: <http://wikiba.se/ontology#>
)";

void printHelp(char* name) {
  std::cout << "Usage: " << name << " <input-file> <query-output-file> <paths-output>" << std::endl;
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
  YAML::Emitter emitter;
  emitter << YAML::BeginMap;
  emitter << YAML::Key << "kb";
  emitter << YAML::Value << set.kb;
  emitter << YAML::Key << "queries";
  emitter << YAML::Value;
  emitter << YAML::BeginSeq;
  for (const Query& q : set.queries) {
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "query";
    emitter << YAML::Value << q.name;

    emitter << YAML::Key << "sparql";
    emitter << YAML::Value << YAML::Literal << q.sparql;

    emitter << YAML::Key << "type";
    emitter << YAML::Value << q.type;
    emitter << YAML::EndMap;
  }
  emitter << YAML::EndSeq;
  emitter << YAML::EndMap;
  std::ofstream f(path);
  f << emitter.c_str();
}

bool doesQueryContainPropertyPath(const Query& q, std::vector<SparqlTriple> *path_triples) {
  bool contains_path = false;
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
          path_triples->push_back(t);
          contains_path = true;
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
  return contains_path;
}

QuerySet queriesFromPaths(const std::vector<SparqlTriple> &paths) {
  QuerySet set;
  set.kb = "wikidata";
  for (const SparqlTriple t : paths) {
    Query q;
    q.name = t.asString();
    q.type = "single path";
    std::ostringstream s;
    s << prefixes;
    s << "SELECT ";
    if (t._s[0] == '?') {
      s << t._s << " ";
    }
    if (t._o[0] == '?') {
      s << t._o << " ";
    }
    s << " WHERE {\n";
    s << t._s << " " << t._p << " " << t._o << "\n";
    s << "}";
    q.sparql = s.str();
    set.queries.emplace_back(q);
  }

  return set;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    printHelp(argv[0]);
    return 1;
  }
  std::vector<SparqlTriple> path_triples;
  QuerySet in = parseQuerySet(std::string(argv[1]));
  QuerySet out;
  out.kb = in.kb;
  for (const Query& q : in.queries) {
    if (doesQueryContainPropertyPath(q, &path_triples)) {
      out.queries.emplace_back(q);
    }
  }
  writeQuerySet(std::string(argv[2]), out);

  QuerySet p = queriesFromPaths(path_triples);
  writeQuerySet(std::string(argv[3]), p);
  return 0;
}
