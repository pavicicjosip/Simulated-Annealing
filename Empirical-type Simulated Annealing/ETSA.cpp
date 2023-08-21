#include "ETSA_header.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>


namespace fs = std::filesystem;

int main() {
  std::string letter = "P";
  unsigned int num_threads = 10;
  int num_multistarts = 10;
  std::string folder_path = "data/" + letter;

  for (const auto& entry : fs::directory_iterator(folder_path)) {
    if (entry.is_regular_file() && entry.path().extension() == ".vrp") {
      std::string PATH = "data/" + letter + "/" + entry.path().filename().string();

      std::cout << "Calculating: " << PATH << std::endl;

      std::vector<VRPInstance> instances; 
      std::vector<std::thread> threads;
      std::vector<TEXT_FILE> results;
      
      for (unsigned int i = 0; i < num_threads; ++i) {
        VRPInstance instance = read_vrp(PATH);
        instances.push_back(instance);
      }

      for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&instances, num_multistarts, &results, i]() {
          TEXT_FILE result = ETSA(instances[i], num_multistarts);
          results.push_back(result);
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      auto it = std::min_element(results.begin(), results.end(), [](const TEXT_FILE& t1, const TEXT_FILE& t2) {
        return t1.best_cost < t2.best_cost;
      });

      float totalDuration = std::accumulate(results.begin(), results.end(), 0.0f,
        [](float sum, const TEXT_FILE& t) {
            return sum + t.duration;
      });

      std::vector<float> COST_100;
      for(const TEXT_FILE& text: results){
        COST_100.insert(COST_100.end(), text.COST_10.begin(), text.COST_10.end());
      }

      save(*it, totalDuration, "solutions/" + letter, COST_100);

    }
  }

  return EXIT_SUCCESS;
}