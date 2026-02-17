#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <thread>
#include <mutex>
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

static void worker(const std::vector<std::string>* level_nodes,
                   size_t begin,
                   size_t end,
                   std::unordered_set<std::string>* visited,
                   std::mutex* visited_mutex,
                   std::vector<std::string>* next_level,
                   std::mutex* next_mutex) {
  CURL* curl = curl_easy_init();
  if (!curl) return;

  std::vector<std::string> local_new;

  for (size_t i = begin; i < end; i++) {
    const std::string& node = (*level_nodes)[i];
    auto neighs = get_neighbors(fetch_neighbors(curl, node));

    for (const auto& neigh : neighs) {
      bool inserted = false;
      {
        std::lock_guard<std::mutex> lock(*visited_mutex);
        inserted = visited->insert(neigh).second;
      }
      if (inserted) local_new.push_back(neigh);
    }
  }

  {
    std::lock_guard<std::mutex> lock(*next_mutex);
    next_level->insert(next_level->end(), local_new.begin(), local_new.end());
  }

  curl_easy_cleanup(curl);
}

int main(int argc, char* argv[]) {
  if (argc < 3 || argc > 4) {
    std::cerr << "Usage: " << argv[0] << " <node_name> <depth> [max_threads]\n";
    return 1;
  }

  std::string start_node = argv[1];
  int depth = std::stoi(argv[2]);

  size_t max_threads = 8;
  if (argc == 4) {
    int mt = std::stoi(argv[3]);
    if (mt > 0) max_threads = (size_t)mt;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  std::unordered_set<std::string> visited;
  std::mutex visited_mutex;

  std::vector<std::vector<std::string>> levels;
  levels.push_back({start_node});
  visited.insert(start_node);

  auto t0 = std::chrono::steady_clock::now();

  for (int d = 0; d < depth; d++) {
    const auto& current = levels[d];
    std::vector<std::string> next;
    std::mutex next_mutex;

    if (current.empty()) {
      levels.push_back({});
      continue;
    }

    size_t threads_to_use = current.size() < max_threads ? current.size() : max_threads;
    if (threads_to_use == 0) threads_to_use = 1;

    size_t n = current.size();
    size_t base = n / threads_to_use;
    size_t rem = n % threads_to_use;

    std::vector<std::thread> threads;
    threads.reserve(threads_to_use);

    size_t idx = 0;
    for (size_t t = 0; t < threads_to_use; t++) {
      size_t chunk = base + (t < rem ? 1 : 0);
      size_t begin = idx;
      size_t end = idx + chunk;
      idx = end;

      threads.emplace_back(worker,
                           &current,
                           begin,
                           end,
                           &visited,
                           &visited_mutex,
                           &next,
                           &next_mutex);
    }

    for (auto& th : threads) th.join();
    levels.push_back(std::move(next));
  }

  auto t1 = std::chrono::steady_clock::now();

  for (size_t d = 0; d < levels.size(); d++) {
    std::cout << "Level " << d << " count: " << levels[d].size() << "\n";
  }

  std::chrono::duration<double> elapsed = t1 - t0;
  std::cout << "Time to crawl (parallel): " << elapsed.count() << "s\n";
  std::cout << "Max threads: " << max_threads << "\n";

  curl_global_cleanup();
  return 0;
}

