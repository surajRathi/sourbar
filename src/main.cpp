#include "../include/log.h"

#include "../include/modules.h"


#include <thread>
#include <functional>
#include <mutex>

#include<map>

#include <fstream>


std::string CONFIG_FILENAME = "config";

bool clean(const std::string &line, size_t &start, size_t &end, const size_t &min_len = 0,
           const std::string &whitespace = " \t") {
    if (start + min_len > end + 1) return false;

    start = line.find_first_not_of(whitespace, start);
    if (start == -1) return false;
    if (line[start] == '#') return false;

    end = line.find_last_not_of(whitespace, end);
    if (end == -1) return false;

    return start + min_len <= end + 1;
}


int main() {
    DEB("Started");

    std::mutex mutex;
    std::vector<std::thread> threads;
    std::vector<std::string> outputs;

    // TODO: Move this out.
    std::ifstream config_file(CONFIG_FILENAME);
    if (!config_file.is_open())
        exit(1);

    std::string line, section, key, value;
    size_t start_index, end_index, pos, key_end, value_start;

    modules::ModuleMap::const_iterator mod_iter;
    modules::Module module_func;
    modules::Options opts;
    modules::Options::const_iterator opt_iter;
    modules::BarMap::const_iterator bar_iter;

    while (std::getline(config_file, line)) {
        start_index = 0;
        end_index = line.length();

        if (!clean(line, start_index, end_index, 3)) continue;
        if ((line[start_index] == '[') && (line[end_index] == ']')) {
            if (!section.empty()) {
                if (module_func != nullptr) {
                    outputs.emplace_back();
                    threads.emplace_back(module_func,
                                         std::ref(mutex), std::ref(outputs.back()), opts);
                    DEB("Started" << section << ".");
                } else {
                    bar_iter = modules::bars.find(section);
                    if (bar_iter != modules::bars.end()) {
                        threads.emplace_back(bar_iter->second, std::ref(mutex), std::ref(outputs), opts);
                        DEB("Started bar: " << section << ".");
                        DEB(opts.at("font-1"));
                    }
                }
                section = "";
            }

            ++start_index;
            --end_index;

            if (!clean(line, start_index, end_index, 1)) continue;
            section = std::string(&line[start_index], &line[end_index + 1]);

            mod_iter = modules::module_map.find(section);
            if (mod_iter == modules::module_map.end()) {
                section = "";
                continue;
            }
            std::tie(module_func, opts) = mod_iter->second; // modules::module_map.at(section);
            DEB("Section " << section << ":");
            continue;
        }

        if (section.empty()) continue;

        pos = line.find('=', start_index);
        if (start_index < pos < end_index) {
            key_end = pos - 1;
            value_start = pos + 1;
            if (!clean(line, start_index, key_end, 1) || !clean(line, value_start, end_index, 1))
                continue;

            if (line[value_start] == '"' && line[end_index] == '"') {
                value_start++;
                end_index--;
            }

            key = std::string(&line[start_index], &line[key_end + 1]);
            value = std::string(&line[value_start], &line[end_index + 1]);
            opt_iter = opts.find(key);
            if (opt_iter == opts.end()) continue;
            opts.at(key) = value;
            DEB(section << "::" << key << " = " << value);
        }
    }
    if (!section.empty()) {
        if (module_func != nullptr) {
            outputs.emplace_back();
            threads.emplace_back(module_func,
                                 std::ref(mutex), std::ref(outputs.back()), opts);
            DEB("Started" << section << ".");
        } else {
            bar_iter = modules::bars.find(section);
            if (bar_iter != modules::bars.end()) {
                threads.emplace_back(bar_iter->second, std::ref(mutex), std::ref(outputs), opts);
                DEB("Started bar: " << section << ".");
            }
        }
        section = "";
    }
    config_file.close();




    // TODO: Signal handling in main thread.
    for (auto &th : threads)
        th.join();
    DEB("Finished");
    return 0;
}