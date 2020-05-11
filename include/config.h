#ifndef BAR_CONFIG_H
#define BAR_CONFIG_H

#include "../include/modules.h"

#include <vector>

#include <thread>
#include <string>
#include <mutex>
#include <shared_mutex>

bool load_config(const char *filename,
                 std::vector<std::thread> &threads,
                 std::vector<std::unique_ptr<std::string>> &outputs,
                 std::vector<std::unique_ptr<modules::Options>> &options,
                 std::mutex &wake_mutex, std::shared_mutex &data_mutex);

#endif //BAR_CONFIG_H