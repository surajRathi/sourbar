#include "../include/modules.h"
#include "../include/subprocess.h"

#include <thread>

#include <ctime>
#include <shared_mutex>

bool modules::is_ok = true;

// TODO: More helper functions like get_duration, get_color, etc.
// TODO: standardized log messages with entire message as one string ( no interleaving)

std::pair<std::string, std::string> colorwrap(const modules::Options &options) {
    std::string prefix, suffix;
    try {  // try-catch not req.
        const std::string &color = options.at("color");
        const std::string &background = options.at("background");
        // TODO: Sanitize and check color. no '%', '{', '}'.

        if (!color.empty()) {
            prefix += "%{F" + color + "}";
            suffix = "%{F-}" + suffix;
        }
        if (!background.empty()) {
            prefix += "%{B" + background + "}";
            suffix = "%{B-}" + suffix;
        }
        return std::make_pair(prefix, suffix);
    } catch (const std::out_of_range &e) {
        return std::make_pair(prefix, suffix);
    }

}

const size_t CLOCK_BUFFER_SIZE = 30;

void modules::clock(std::mutex &wake_mutex, std::shared_mutex &data_mutex,
                    std::string &output, const Options &options) {
    const auto&[prefix, suffix] = colorwrap(options);

    const char *format = options.at("format").data();
    char buffer[CLOCK_BUFFER_SIZE];
    std::time_t time;

    std::chrono::duration<int64_t> interval = std::chrono::seconds(1);
    try {
        // TODO: add float intervals -> milliseconds
        interval = std::chrono::seconds(std::stol(options.at("interval")));
    } catch (const std::exception &e) { // std::invalid_argument & std::out_of_range
        //interval = std::chrono::seconds(1);
        ERR("Invalid duration: " << options.at("interval"));
    }

    INFO("[clock@" << (size_t) &options % 1000 << "] " << "Started");

    while (modules::is_ok) {
        time = std::time(nullptr);
        if (!std::strftime(buffer, CLOCK_BUFFER_SIZE - 1, format, std::localtime(&time))) {
            ERR("[clock@" << (size_t) &options % 1000 << "] " << "Cant format time.");
            modules::update_function(wake_mutex, data_mutex, output); // ,"");
            return;
        }

        modules::update_function(wake_mutex, data_mutex, output,
                                 prefix + buffer + suffix);

        std::this_thread::sleep_for(interval);
    }
    INFO("[clock@" << (size_t) &options % 1000 << "] " << "Done");
}

void modules::text(std::mutex &wake_mutex, std::shared_mutex &data_mutex,
                   std::string &output, const modules::Options &options) {
    const auto&[prefix, suffix] = colorwrap(options);

    INFO("[text@" << (size_t) &options % 1000 << "] " << "Started");

    modules::update_function(wake_mutex, data_mutex, output,
                             prefix + options.at("text") + suffix);

    INFO("[text@" << (size_t) &options % 1000 << "] " << "Done");
}

// TODO: Doesnt allow += or append..
void modules::update_function(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output,
                              const std::string value) {
    std::shared_lock<std::shared_mutex> lock(data_mutex);
    output = value;
    wake_mutex.unlock();
}
