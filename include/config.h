#ifndef BAR_CONFIG_H
#define BAR_CONFIG_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>

void load_config(const char *filename, std::vector<std::thread> &threads, std::vector<std::string> &outputs,
                 std::mutex &mutex);

#endif //BAR_CONFIG_H