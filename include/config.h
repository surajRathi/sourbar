#ifndef BAR_CONFIG_H
#define BAR_CONFIG_H

#include "../include/modules.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>

void load_config(const char *filename, std::vector<std::thread> &threads, std::vector<std::string> &outputs, std::vector<modules::Options> & options,
                 std::mutex &mutex);

#endif //BAR_CONFIG_H