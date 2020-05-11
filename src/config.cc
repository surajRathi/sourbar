#include "../include/config.h"
#include <fstream>

// Convert parse process to use string_view on each line.
bool clean(const std::string &line, size_t &start, size_t &end, const size_t &min_len = 0,
           const char *whitespace = " \t") {
    if (start + min_len > end + 1) return false;

    start = line.find_first_not_of(whitespace, start);
    if (start == -1) return false;
    if (line[start] == '#') return false;

    end = line.find_last_not_of(whitespace, end);
    if (end == -1) return false;

    return start + min_len <= end + 1;
}

// TODO : rewrite with string view.
// TODO : Support comments at the end of line, while respecting '#' in strings ( "<foo>" )
bool load_config(const char *filename,
                 std::vector<std::thread> &threads,
                 std::vector<std::unique_ptr<std::string>> &outputs,
                 std::vector<std::unique_ptr<modules::Options>> &options,
                 std::mutex &wake_mutex, std::shared_mutex &data_mutex) {

    std::ifstream config_file(filename);
    if (!config_file.is_open()) {
        ERR("Cant open config_file: " << filename);
        return false;
    }

    bool bar = false;

    // Simplify with string_view
    std::string line, section, key, value;
    size_t start_index, end_index, pos, key_end, value_start;

    modules::ModuleMap::const_iterator mod_iter;
    modules::Module module_func = nullptr;
    modules::Options::iterator opt_iter;
    modules::BarMap::const_iterator bar_iter;

    while (std::getline(config_file, line)) {
        start_index = 0;
        end_index = line.length();

        if (!clean(line, start_index, end_index, 3)) continue;
        if ((line[start_index] == '[') && (line[end_index] == ']')) {
            if (!section.empty()) {
                if (module_func != nullptr) {
                    outputs.push_back(std::make_unique<std::string>());
                    threads.emplace_back(module_func, std::ref(wake_mutex), std::ref(data_mutex),
                                         std::ref(*outputs.back()),
                                         std::ref(*options.back()));
                } else if (!bar) {
                    bar_iter = modules::bars.find(section);
                    if (bar_iter != modules::bars.end()) {
                        threads.emplace_back(bar_iter->second, std::ref(wake_mutex), std::ref(data_mutex),
                                             std::ref(outputs),
                                             std::ref(*options.back()));
                        bar = true;
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
            auto &_x = mod_iter->second; // modules::module_map.at(section);
            module_func = _x.first;
            options.push_back(std::make_unique<modules::Options>(_x.second));

            INFO("Found module: " << section);
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
            opt_iter = (*options.back()).find(key);
            if (opt_iter == (*options.back()).end()) continue;
            opt_iter->second = value;
        }
    }
    if (!section.empty()) { // TODO: move to function repeated from loop.
        if (module_func != nullptr) {
            outputs.emplace_back();
            threads.emplace_back(module_func, std::ref(wake_mutex), std::ref(data_mutex), std::ref(*outputs.back()),
                                 std::ref(*options.back()));
        } else if (!bar) {
            bar_iter = modules::bars.find(section);
            if (bar_iter != modules::bars.end()) {
                threads.emplace_back(bar_iter->second, std::ref(wake_mutex), std::ref(data_mutex), std::ref(outputs),
                                     std::ref(*options.back()));
                bar = true;
            }
        }
        section = "";
    }

    config_file.close();

    if (!bar) ERR("No bar started.");

    return bar;
}
