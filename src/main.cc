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
    if ((config_home = std::getenv("XDG_CONFIG_HOME"))) {

        const auto config_home_str = std::string(config_home);
        config_files.push_back(config_home_str + "/sourbar/config");
        config_files.push_back(config_home_str + "/sourbarrc");
    }

    char *home;
    if ((home = std::getenv("HOME"))) {
        const auto home_str = std::string(home);
        config_files.push_back(home_str + "/.config/sourbar/config");
        config_files.push_back(home_str + "/.config/sourbarrc");
        config_files.push_back(home_str + "/.sourbarrc");
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

    // TODO: Handle signals.

    threads.back().join();
    ERR("Lemonbar joined!."); // ?
    return 0;

    /*for (auto &th : threads)
        th.join();

    DEB("Finished.");
    return 0;*/
}


bool clean(std::string_view &line, const size_t &min_len = 0,
           const char *whitespace = " \t") {
    if (line.length() < min_len) return false;

    line.remove_prefix(line.find_first_not_of(whitespace));

    if (line.length() < min_len) return false;
    if (line.front() == '#') return false;

    line.remove_suffix(line.length() - line.find_last_not_of(whitespace) - 1);
    return line.length() >= min_len;

}

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

    std::string line_str, section_name;
    modules::Module module_func = nullptr;

    auto add_section = [&]() {
        if (module_func != nullptr) {
            outputs.push_back(std::make_unique<std::string>());
            threads.emplace_back(
                    module_func,
                    // TODO: Better approach?
                    std::move(std::bind(modules::update_function,
                                        std::ref(wake_mutex), std::ref(data_mutex),
                                        std::ref(*outputs.back()), std::placeholders::_1)),
                    std::ref(*options.back()));
        } else if (!bar) {
            auto bar_iter = modules::bars.find(section_name);
            if (bar_iter != modules::bars.end()) {
                threads.emplace_back(bar_iter->second, std::ref(wake_mutex), std::ref(data_mutex),
                                     std::ref(outputs),
                                     std::ref(*options.back()));
                bar = true;
            }
        }
        section_name.clear();
    };

    while (std::getline(config_file, line_str)) {
        std::string_view line{line_str};


        if (!clean(line, 3)) continue;
        if ((line.front() == '[') && (line.back() == ']')) { // New Section
            if (!section_name.empty()) {
                add_section();
            }

            // Ignore '[',']'
            line.remove_prefix(1);
            line.remove_suffix(1);

            if (!clean(line, 1)) continue;
            section_name = line; // copy

            auto mod_iter = modules::module_map.find(section_name);
            if (mod_iter == modules::module_map.end()) {
                section_name.clear();
                continue;
            }
            auto &_x = mod_iter->second; // modules::module_map.at(section_name);
            module_func = _x.first;
            options.push_back(std::make_unique<modules::Options>(_x.second));

            // INFO("Found module: " << section_name);
            continue;
        }

        if (section_name.empty()) continue;

        const auto pos = line.find('=');

        if (pos != std::string::npos) {
            std::string_view key{line}, val{line};
            val.remove_prefix(pos + 1);
            key.remove_suffix(key.length() - pos);

            if (!clean(key, 1) || !clean(val, 1)) {
                continue;
            }

            if (val.front() == '"' && val.back() == '"') {
                val.remove_prefix(1);
                val.remove_suffix(1);
            }

            auto opt_iter = (*options.back()).find(std::string{key});
            if (opt_iter == (*options.back()).end()) continue;
            opt_iter->second = val;
        }
    }

    if (!section_name.empty())
        add_section();


    config_file.close();

    if (!bar) ERR("No bar started.");

    return bar;
}

#include <dlfcn.h>