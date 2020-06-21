#include "../include/log.h"
#include "../include/modules.h" // for modules::Options

#include <thread>

#include <mutex>
#include <shared_mutex>

#include <string>
#include <vector>

#include <filesystem>
#include <fstream>

bool load_config(const std::string &filename,
                 std::vector<std::thread> &threads,
                 std::vector<std::unique_ptr<std::string>> &outputs,
                 std::vector<std::unique_ptr<modules::Options>> &options,
                 std::mutex &wake_mutex, std::shared_mutex &data_mutex);

int main() {
    INFO("Starting up.");

#ifndef NDEBUG
    const char *const config_files[] = {"config", "/home/suraj/.config/sourbarrc"};
#else
    std::vector<std::string> config_files{"sourbarrc"};

    char *config_home;
    if ((config_home = std::getenv("XDG_CONFIG_HOME")) != nullptr) {
        config_files.push_back(std::string(config_home) + "/sourbar/config");
        config_files.push_back(std::string(config_home) + "/sourbarrc");
    }

    char *home = nullptr;
    if ((home = std::getenv("HOME")) != nullptr) {
        config_files.push_back(std::string(home) + "/.config/sourbar/config");
        config_files.push_back(std::string(home) + "/.config/sourbarrc");
        config_files.push_back(std::string(home) + "/.sourbarrc");
    }
#endif

    // TODO: Handle command line options

    std::mutex wake_mutex;
    std::shared_mutex data_mutex;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<std::string>> outputs;
    // ^ Do I need this in main? If i move it to config will it destroy strings at end of config func?
    std::vector<std::unique_ptr<modules::Options>> options; // Do I really need an vector of options?

    bool initialized = false;
    for (auto &filename : config_files) {
        if (std::filesystem::exists(std::filesystem::path(filename))) {
            INFO("Using config at: " << filename << ".");
            initialized = load_config(filename, threads, outputs, options, wake_mutex, data_mutex);
            break;
        }
    }
    if (!initialized) {
        ERR("Not initialized.");
        exit(1);
    }

    // Handle signals.

    threads.back().join();
    ERR("Lemonbar joined!.");
    return 0;
    for (auto &th : threads)
        th.join();

    DEB("Finished.");
    return 0;
}


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
bool load_config(const std::string &filename,
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

    auto add_section = [&]() {
        if (module_func != nullptr) {
            outputs.push_back(std::make_unique<std::string>());
            threads.emplace_back(
                    module_func,
                    // TODO : Move to lambda.
                    std::move(std::bind(modules::update_function,
                                        std::ref(wake_mutex), std::ref(data_mutex),
                                        std::ref(*outputs.back()), std::placeholders::_1)),
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
    };

    while (std::getline(config_file, line)) {
        start_index = 0;
        end_index = line.length();


        if (!clean(line, start_index, end_index, 3)) continue;
        if ((line[start_index] == '[') && (line[end_index] == ']')) {
            if (!section.empty()) {
                add_section();
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

            // INFO("Found module: " << section);
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

    if (!section.empty())
        add_section();


    config_file.close();

    if (!bar) ERR("No bar started.");

    return bar;
}

#include <dlfcn.h>