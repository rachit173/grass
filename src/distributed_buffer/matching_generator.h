#ifndef MATCHING_GENERATOR_H
#define MATCHING_GENERATOR_H

#include <vector>
#include <utility>
#include <assert.h>

typedef std::vector<std::vector<std::pair<int, int>>> vvii;

void GenerateMatchings(int l, int r, std::vector<std::vector<std::pair<int, int>>>& matchings);
void GeneratePlan(std::vector<std::vector<std::pair<int, int>>>& matchings, 
                  std::vector<std::vector<std::pair<int, int>>>& plan, 
                  std::vector<std::vector<std::pair<int, int>>>& machine_state,
                  std::vector<std::vector<int>>& partition_to_be_sent);

#endif // MATCHING_GENERATOR_H