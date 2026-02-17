
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <chrono>
#include <curl/curl.h>

#include "rapidjson/document.h"

static const std::string SERVICE_URL =
    "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
  size_t totalSize = size * nmemb;
  output->append((char*)contents, totalSize);
  return totalSize;
}

static std::string url_encode(CURL* curl, const std::string& input) {
  char* out = curl_easy_escape(curl, input.c_str(), (int)input.size());
  std::string s = out ? out : "";
  curl_free(out);
  return s;
}

static std::string fetch_neighbors(CURL* curl, const std::string& node) {
  std::string url = SERVICE_URL + url_encode(curl, node);
  std::string response;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) return "{}";
  return response;
}

static std::vector<std::string> get_neighbors(const std::string& json_str) {
  std::vector<std::string> neighbors;
  rapidjson::Document doc;
  doc.Parse(json_str.c_str());

  if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
    for (const auto& n : doc["neighbors"].GetArray()) {
      neighbors.push_back(n.GetString());
    }
  }
  return neighbors;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <node_name> <depth>\n";
    return 1;
  }

  std::string start_node = argv[1];
  int depth = std::stoi(argv[2]);

  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to init curl\n";
    curl_global_cleanup();
    return 1;
  }

  std::unordered_set<std::string> visited;
  std::vector<std::vector<std::string>> levels;
  levels.push_back({start_node});
  visited.insert(start_node);

  auto t0 = std::chrono::steady_clock::now();

  for (int d = 0; d < depth; d++) {
    levels.push_back({});
    for (const auto& node : levels[d]) {
      for (const auto& neigh : get_neighbors(fetch_neighbors(curl, node))) {
        if (!visited.count(neigh)) {
          visited.insert(neigh);
          levels[d + 1].push_back(neigh);
        }
      }
    }
  }

  auto t1 = std::chrono::steady_clock::now();

  for (size_t d = 0; d < levels.size(); d++) {
    std::cout << "Level " << d << " count: " << levels[d].size() << "\n";
  }

  std::chrono::duration<double> elapsed = t1 - t0;
  std::cout << "Time to crawl (sequential): " << elapsed.count() << "s\n";

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return 0;
}
