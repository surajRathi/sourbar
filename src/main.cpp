#include "../include/log.h"
#include "../include/config.h"
#include "../include/modules.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include <filesystem>

// termite looks for the configuration file in the following order: "$XDG_CONFIG_HOME/termite/config", "~/.config/termite/config", "$XDG_CONFIG_DIRS/termite/config" and "/etc/xdg/termite/config"
const char *const config_files_hierarchy[] = {
        "config" /*., "/barname.conf", "$XDG_CONFIG_DIRECTORY/barname/config", "$XDG_HOME/.barnamerc", "/etc/barname.conf"*/};

int main() {
    DEB("Started");

    std::mutex mutex;
    std::vector<std::thread> threads;
    std::vector<std::string> outputs;
    std::vector<modules::Options> options;

    for (auto &filename : config_files_hierarchy) {
        if (std::filesystem::exists(std::filesystem::path(filename))) {
            load_config(filename, threads, outputs, options, mutex);
            break;
        }
    }

    // TODO: Signal handling in main thread.
    for (auto &th : threads)
        th.join();

    DEB("Finished");
    return 0;
}