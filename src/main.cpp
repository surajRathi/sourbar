#include "../include/log.h"

#include "../include/config.h"
#include "../include/modules.h" // for modules::Options

#include <string>
#include <vector>

#include <thread>
#include <mutex>
#include <shared_mutex>

#include <filesystem>

// termite looks for the configuration file in the following order: "$XDG_CONFIG_HOME/termite/config", "~/.config/termite/config", "$XDG_CONFIG_DIRS/termite/config" and "/etc/xdg/termite/config"
const char *const config_files_hierarchy[] = {
        "config" /*., "/barname.conf", "$XDG_CONFIG_DIRECTORY/barname/config", "$XDG_HOME/.barnamerc", "/etc/barname.conf"*/};

int main() {
    INFO("Starting up.");

    // Handle command line options

    std::mutex wake_mutex;
    std::shared_mutex data_mutex;
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<std::string>> outputs;
    // ^ Do I need this in main? If i move it to config will it destroy strings at end of config func?
    std::vector<std::unique_ptr<modules::Options>> options; // Do I really need an vector of options?

    bool initialized = false;
    for (auto &filename : config_files_hierarchy) {
        if (std::filesystem::exists(std::filesystem::path(filename))) {
            initialized = load_config(filename, threads, outputs, options, wake_mutex, data_mutex);
            break;
        }
    }
    if (!initialized) {
        ERR("Not initialized.");
        exit(1);
    }

    // Handle signals.

    for (auto &th : threads)
        th.join();

    DEB("Finished.");
    return 0;
}