#ifndef SA_FUNC_H
#define SA_FUNC_H

#include <vector>
#include <string>
#include <map>


struct Customer {
    int id;
    int x;
    int y;
    int demand;
};

struct VRPInstance {
    std::string NAME;
    std::string COMMENT;
    int K;
    int OPT;
    std::string TYPE;
    int DIMENSION;
    std::string EDGE_WEIGHT_TYPE;
    int CAPACITY;
    Customer DEPOT;
    std::vector<Customer> NODE_COORD_SECTION;
    std::map<int, Customer> dict;

    std::vector<int> S;
    std::vector<int> S_best;
    std::vector<int> S_start;
    std::vector<float> TEMPERATURE;
    std::vector<float> COST;
    std::vector<int> N;
};

struct TEXT_FILE {
    std::string NAME;
    int K;
    int OPT;
    int CAPACITY;

    float best_cost;
    float duration;
    std::vector<float> COST_10;
    std::vector<int> S;
    std::vector<int> S_best;
    std::vector<int> S_start;
    std::vector<float> TEMPERATURE;
    std::vector<float> COST;
};

VRPInstance read_vrp(const std::string PATH);
float EUC_2D(const Customer& p, const Customer& q);
bool is_solution_valid(const VRPInstance& instance);
bool is_neighbour_valid(const VRPInstance& instance);
void generate_S(VRPInstance &instance);
float cost_S(const VRPInstance &instance);
float cost_S_best(const VRPInstance &instance);
float cost_N(const VRPInstance &instance);
bool generate_N(VRPInstance& instance);
void save(const TEXT_FILE& txt, float duration, std::string PATH, const std::vector<float>& COST_100);
TEXT_FILE SA(VRPInstance& instance, int num_multistarts);
#endif