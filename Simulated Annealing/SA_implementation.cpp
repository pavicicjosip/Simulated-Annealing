#include "SA_header.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cmath>
#include <chrono>
#include <iomanip>

std::string remove_spaces(const std::string input) {
    std::string result;

    for (char c : input) {
        if (c != '\t') {
            result += c;
        } else {
            result += ' ';
        }
    }

    result.erase(std::unique(result.begin(), result.end(),
                             [](char a, char b) { return a == ' ' && b == ' '; }),
                 result.end());

    return result;
}

VRPInstance read_vrp(const std::string PATH) {
    VRPInstance obj;
    std::ifstream file(PATH);
    if (!file.is_open()) {
        std::cerr << "Error opening the file: " << PATH << std::endl;
        return obj;
    }

    std::string line;
    std::vector<std::string> rows;

    while (std::getline(file, line)) {
        rows.push_back(remove_spaces(line));
    }

    obj.NAME = rows[0].substr(rows[0].find(':') + 2);
    obj.COMMENT = rows[1].substr(rows[1].find(':') + 2);
    obj.K = std::stoi(rows[2].substr(rows[2].find(':') + 2));
    obj.OPT = std::stoi(rows[3].substr(rows[3].find(':') + 2));
    obj.TYPE = rows[4].substr(rows[4].find(':') + 2);
    obj.DIMENSION = std::stoi(rows[5].substr(rows[5].find(':') + 2));
    obj.EDGE_WEIGHT_TYPE = rows[6].substr(rows[6].find(':') + 2);
    obj.CAPACITY = std::stoi(rows[7].substr(rows[7].find(':') + 2));

    for (size_t i = 9; i < 9 + obj.DIMENSION; ++i) {
        int num, x, y;
        std::istringstream iss_coords(rows[i]);
        if (!(iss_coords >> num >> x >> y)) {
            std::cerr << "Error parsing line " << i << " in NODE_COORD_SECTION." << std::endl;
            continue; 
        }

        int num2, demand;
        std::istringstream iss_demand(rows[i + obj.DIMENSION + 1]);
        if (!(iss_demand >> num2 >> demand)) {
            std::cerr << "Error parsing line " << i << " in DEMAND_SECTION." << std::endl;
            continue; 
        }

        Customer c {num - 1, x, y, demand};
        obj.NODE_COORD_SECTION.push_back(c);
    }

    obj.DEPOT = obj.NODE_COORD_SECTION[0];
    for (auto& customer : obj.NODE_COORD_SECTION) {
      obj.dict.insert(std::pair<int, Customer>(customer.id, customer));
    }

    obj.NODE_COORD_SECTION.erase(obj.NODE_COORD_SECTION.begin());
    return obj;
}

float EUC_2D(const Customer& p, const Customer& q) {
  return sqrt(pow(q.x-p.x, 2) + pow(q.y-p.y,2));
}

bool is_neighbour_valid(const VRPInstance& instance){
  int Q = instance.CAPACITY;
  for (int id : instance.N) {
    if (id == 0) {
      if ( Q < 0) {
        return false;
      } else {
        Q = instance.CAPACITY;
      }
    } else {
      Q -= instance.dict.at(id).demand;
    }    
  }
  return true;
}

bool is_solution_valid(const VRPInstance& instance){
  int Q = instance.CAPACITY;
  for (int id : instance.S) {
    if (id == 0) {
      if ( Q < 0) {
        return false;
      } else {
        Q = instance.CAPACITY;
      }
    } else {
      Q -= instance.dict.at(id).demand;
    }    
  }
  return true;
}

void generate_S(VRPInstance &instance) {
  for (int i = 0; i < 100; ++i) {
    std::vector<int> routes_capacity(instance.K, instance.CAPACITY);
    std::vector<Customer> customers (instance.NODE_COORD_SECTION.begin(), instance.NODE_COORD_SECTION.end());

    unsigned seed = std::chrono::system_clock::now()
                    .time_since_epoch()
                    .count();
    std::shuffle(std::begin(customers), std::end(customers), std::default_random_engine(seed));
    
    std::vector<std::vector<Customer>> S(instance.K);
    for (auto& customer : customers) {
      for (int k = 0; k < instance.K; ++k) {
        if(routes_capacity[k] >= customer.demand) {
          routes_capacity[k] -= customer.demand;
          S[k].push_back(customer);
          break;
        }
      }
    }
    
    for (auto& route : S) {
      instance.S.push_back(0);
      instance.S_start.push_back(0);
      for(auto& customer : route) {
        instance.S.push_back(customer.id);
        instance.S_start.push_back(customer.id);
      }
    }
    instance.S.push_back(0);
    instance.S_start.push_back(0);


    if(is_solution_valid(instance)) {
      for( auto i : instance.S) {
        instance.S_best.push_back(i);
      }
      break;
    } else {
      instance.S.clear();
      instance.S_start.clear();
    }
  }
}

float cost_S(const VRPInstance &instance) {
  float sol = 0.0f;
  for (int i = 0; i < instance.S.size() - 1; ++i){
    sol += EUC_2D(instance.dict.at(instance.S.at(i)), instance.dict.at(instance.S.at(i + 1)));
  }
  return sol;
}

float cost_S_best(const VRPInstance &instance) {
  float sol = 0.0f;
  for (int i = 0; i < instance.S_best.size() - 1; ++i){
    sol += EUC_2D(instance.dict.at(instance.S_best.at(i)), instance.dict.at(instance.S_best.at(i + 1)));
  }
  return sol;
}

float cost_N(const VRPInstance &instance) {
  float sol = 0.0f;
  for (int i = 0; i < instance.N.size() - 1; ++i){
    sol += EUC_2D(instance.dict.at(instance.N.at(i)), instance.dict.at(instance.N.at(i + 1)));
  }
  return sol;
}

bool generate_N(VRPInstance& instance) {
  std::vector<std::string> actions{  "insert", "swap", "reversion" };
  int counter = 0;
  while (counter < 1000) {
    ++counter;

    instance.N  = instance.S;
    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> action_dist(0, actions.size() - 1);

    int action_idx = action_dist(gen);
    std::string action = actions[action_idx]; 

    std::uniform_int_distribution<int> dist(1, instance.N.size() - 2);
    int idx1 = dist(gen);
    int idx2;
    do {
        idx2 = dist(gen);
    } while (idx2 == idx1);

    if(action == "swap") {
      std::swap(instance.N[idx1], instance.N[idx2]);
    } else if (action == "reversion") {
      std::reverse(instance.N.begin() + idx1, instance.N.begin() + idx2 + 1);
    } else if (action == "insert") {
      int elementToMove = instance.N[idx1];
      instance.N.erase(instance.N.begin() + idx1);

      int insertIndex = (idx2 < idx1) ? (idx2 + 1) : idx2;
      instance.N.insert(instance.N.begin() + insertIndex, elementToMove);
    }

    if(is_neighbour_valid(instance)) {
      return true;
    }

  }
  return false;
}

void save(const TEXT_FILE& text_file, float duration, std::string PATH, const std::vector<float>& COST_100) {
  std::ofstream outputFile(PATH + "/" + text_file.NAME + ".txt");

  if(!outputFile) {
    std::cout << "file_error: " + PATH + "/" + text_file.NAME + ".txt" << std::endl;
  } else { 
    outputFile << "NAME: ";
    outputFile << text_file.NAME;
    outputFile << std::endl;

    outputFile << "K: ";
    outputFile << text_file.K;
    outputFile << std::endl;

    outputFile << "OPT: ";
    outputFile << text_file.OPT;
    outputFile << std::endl;

    outputFile << "best_cost: ";
    outputFile << std::fixed << std::setprecision(2) << text_file.best_cost;
    outputFile << std::endl;

    float sum = 0;
    for (float num : COST_100) {
        sum += num;
    }

    float avg_cost = sum/static_cast<float>(COST_100.size());
    outputFile << "avg_cost: ";
    outputFile <<  avg_cost;
    outputFile << std::endl;

    outputFile << "duration(s): ";
    outputFile <<  duration;
    outputFile << std::endl;

    float absolute_error = text_file.best_cost - text_file.OPT;
    outputFile << "absolute error: ";
    outputFile <<  absolute_error;
    outputFile << std::endl;

    float relative_error = absolute_error/text_file.OPT;
    outputFile << "relative error(%): ";
    outputFile << relative_error*100;
    outputFile << std::endl;

    float sum_ = 0.0f;
    for(int i = 0; i < COST_100.size(); i++) {
      sum_ += pow(COST_100[i] - avg_cost, 2);
    }
    float variance = (1.0f/(COST_100.size()-1))*sum_;
    outputFile << "variance: ";
    outputFile << variance;
    outputFile << std::endl;

    outputFile << "standard_deviation: ";
    outputFile << sqrt(variance);
    outputFile << std::endl;

    outputFile << "latex row in table (name, capacity, opt, best, avg, absolute error, relative error, standadrd deviation, time)" << std::endl;
    outputFile << text_file.NAME << " & " << text_file.CAPACITY << " & " << text_file.OPT 
               << " & " <<  text_file.best_cost 
               << " & " <<  avg_cost 
               << " & " <<  absolute_error
               << " & " <<  relative_error*100
               << " & " <<  sqrt(variance)
               << " & " <<  duration << " \\\\" << std::endl;



    outputFile << "S: ";
    for(int i = 0; i < text_file.S.size(); ++i){
      outputFile << text_file.S[i] << " ";
    }
    outputFile << std::endl;

    outputFile << "S_best: ";
    for(int i = 0; i < text_file.S_best.size(); ++i){
      outputFile << text_file.S_best[i] << " ";
    }
    outputFile << std::endl;

    outputFile << "S_start: ";
    for(int i = 0; i < text_file.S_start.size(); ++i){
      outputFile << text_file.S_start[i] << " ";
    }
    outputFile << std::endl;

    outputFile << "COST_100: ";
    for(int i = 0; i < COST_100.size(); ++i){
      outputFile << COST_100[i] << " ";
    }
    outputFile << std::endl;


    outputFile << "TEMPERATURE: ";
    for(int i = 0; i < text_file.TEMPERATURE.size(); ++i){
      outputFile << std::fixed << std::setprecision(4) << text_file.TEMPERATURE[i] << " ";
    }
    outputFile << std::endl;

    outputFile << "COST: ";
    for(int i = 0; i < text_file.COST.size(); ++i){
      outputFile << std::fixed << std::setprecision(2) << text_file.COST[i] << " ";
    }
    outputFile << std::endl;

    
  }
  outputFile.close();
}

TEXT_FILE SA(VRPInstance& instance, int num_multistarts) {
  auto start_time = std::chrono::high_resolution_clock::now();
  TEXT_FILE text_file;
  text_file.best_cost = 9999999.0f;
  text_file.NAME = instance.NAME;
  text_file.K = instance.K;
  text_file.OPT = instance.OPT;
  text_file.CAPACITY = instance.CAPACITY;

  for(int j = 0; j < num_multistarts; ++j) {
    instance.S.clear();
    instance.S_best.clear();
    instance.S_start.clear();
    instance.TEMPERATURE.clear();
    instance.COST.clear();
    instance.N.clear();
    
    generate_S(instance);
    float T = 2000;
    float alpha = 0.95;
    float T_final = 0.001;
    int N = 100;

    while (T > T_final) {
      for(int i = 0; i < N; ++i) {
        bool b = generate_N(instance);
        if (cost_S(instance) > cost_N(instance)) {
          instance.S = instance.N;
          // instance.COST.push_back(cost_S(instance));
        } else {
          float delta = cost_N(instance) - cost_S(instance);
          std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
          std::uniform_real_distribution<double> dis(0.0, 1.0);
          float r = dis(gen);
          float exp = std::exp(-delta/T);
          if (r < exp) {
            instance.S = instance.N;
            // instance.COST.push_back(cost_S(instance));
          }
        }
        instance.COST.push_back(cost_S(instance));

        if(cost_S(instance) < cost_S_best(instance)){
          instance.S_best = instance.S;
        }
      }
      instance.TEMPERATURE.push_back(T);
      T *= alpha;
    }

    text_file.COST_10.push_back(cost_S(instance));
    if(text_file.best_cost > cost_S_best(instance)) {
      text_file.best_cost = cost_S_best(instance);
      text_file.S = instance.S;
      text_file.S_best = instance.S_best;
      text_file.S_start = instance.S_start;
      text_file.TEMPERATURE = instance.TEMPERATURE;
      text_file.COST = instance.COST;
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  float duration = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()) / 1000000.0f;

  text_file.duration = duration;
  return text_file;
}