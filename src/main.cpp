#include "../include/log.h"

#include "../include/subprocess.h"
#include "../include/modules.h"


#include <thread>
#include <functional>
#include <mutex>

#include<map>

#include <fstream>


const char *const lemon_cmd[] = {"lemonbar", "-f", "-misc-dejavu sans-medium-r-normal--0-100-0-0-p-9-ascii-0", "-f",
                                 "-wuncon-siji-medium-r-normal--0-0-75-75-c-0-iso10646-1", "-F", "#F5F6F7", "-B",
                                 "#303642"};


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
    LOG("Started");

    std::mutex mutex;

    std::vector<std::thread> threads;
    std::vector<std::string> outputs;

    std::ifstream config_file(CONFIG_FILENAME);
    if (!config_file.is_open())
        exit(1);

    std::string line, section = "", key, value;
    size_t start_index, end_index, pos, key_end, value_start;

    module::ModuleMap::const_iterator mod_iter;
    module::Module module_func;
    module::OptionsMap opts;
    module::OptionsMap::const_iterator opt_iter;

    while (std::getline(config_file, line)) {
        start_index = 0;
        end_index = line.length();

        if (!clean(line, start_index, end_index, 3)) continue;
        if ((line[start_index] == '[') && (line[end_index] == ']')) {
            if (!section.empty()) {
                outputs.emplace_back();
                threads.emplace_back(module_func,
                                     std::ref(mutex), std::ref(outputs.back()), std::ref(opts));
                LOG("Started" << section << ".");
                section = "";
            }

            ++start_index;
            --end_index;

            if (!clean(line, start_index, end_index, 1)) continue;
            section = std::string(&line[start_index], &line[end_index + 1]);

            mod_iter = module::module_map.find(section);
            if (mod_iter == module::module_map.end()) {
                section = "";
                continue;
            }

            std::tie(module_func, opts) = module::module_map.at(section);
            LOG("Section " << section << ":");
            continue;
        }
        if (section.empty()) continue;

        pos = line.find('=', start_index);
        if (start_index < pos < end_index) {
            key_end = pos - 1;
            value_start = pos + 1;
            if (!clean(line, start_index, key_end, 1) || !clean(line, value_start, end_index, 1))
                continue;

            key = std::string(&line[start_index], &line[key_end + 1]);
            value = std::string(&line[value_start], &line[end_index + 1]);
            opt_iter = opts.find(key);
            if (opt_iter == opts.end()) continue;

            opts.at(key) = value;
            LOG(section << "::" << key << " = " << value);
        }

    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    Subprocess s(lemon_cmd);

    while (true) {
        mutex.lock();
        s.stdin << outputs[0] << std::endl;
        LOG("Tick");
    }
    s.send_eof();
    s.wait();


    LOG("Finished");
    return 0;
}