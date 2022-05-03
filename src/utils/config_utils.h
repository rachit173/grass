#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <bits/stdc++.h>

using namespace std;

// Parse config file
int parse_config_file(const std::string &config_file, std::unordered_map<std::string, std::string>& config) {
    std::ifstream file(config_file, std::ios::in);
    std::string line, key, value;

    printf("Parsing config file: %s\n", config_file.c_str());
    if (file.is_open()) {
        while (getline(file, line)) {
            if (line[0] == '#' || line.length() == 0) {
                continue;
            }

            int pos = line.find("=");
            key = line.substr(0, pos);
            value = line.substr(pos + 1);

            config[key] = value;
        }
        file.close();
    } else {
        std::cout << "Unable to open file " << config_file << std::endl;
        return -1;
    }

    return 0;
}

vector<string> split_addresses(string addresses) {
  vector<string> result;
  stringstream ss(addresses);
  string item;
  while (getline(ss, item, ',')) {
    result.push_back(item);
  }
  return result;
}

#endif // CONFIG_UTILS_H