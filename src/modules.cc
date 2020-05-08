#include "../include/modules.h"

#include "../include/log.h"
#include "../include/subprocess.h"

#include <thread>
#include <mutex>

#include <ctime>

const size_t CLOCK_BUFFER_SIZE = 20;

void modules::clock(std::mutex &mutex, std::string &output, const Options options) {
    for (auto &key : options)
        DEB("BB" << key.first << key.second);
    DEB("");


    const char *format = options.at("format").data();
    std::string prefix, suffix;

    {
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
    }

    char buffer[CLOCK_BUFFER_SIZE];
    std::time_t time;

    std::chrono::duration<int64_t> interval = std::chrono::seconds(1);
    try {
        // TODO: add float intervals -> milliseconds
        //float opt = std::atof(options.at("interval").data());
        //interval = std::chrono::milliseconds(100 * opt);
        interval = std::chrono::seconds(std::stol(options.at("interval")));
    } catch (const std::exception &e) { // std::invalid_argument & std::out_of_range
        //interval = std::chrono::seconds(1);
        ERR("Invalid duration: " << options.at("interval"));
    }

    INFO("[clock] Started");

    while (true) {
        time = std::time(nullptr);
        if (!std::strftime(buffer, CLOCK_BUFFER_SIZE, format, std::localtime(&time)))
            continue;
        output = prefix + buffer + suffix; // TODO: check.

        mutex.unlock();
        std::this_thread::sleep_for(interval);
    }
}


void modules::lemonbar(std::mutex &mutex, std::vector<std::string> &outputs, const modules::Options options) {
    for (auto &key : options)
        DEB("AA" << key.first << key.second);
    DEB("");

    const char *const lemon_cmd[] = {
            "lemonbar",
            "-n", options.at("name").data(),
            "-f", options.at("font-1").data(),
            "-f", options.at("font-2").data(),
            "-F", options.at("color").data(),
            "-B", options.at("background").data()
    };

    Subprocess s(lemon_cmd);

    INFO("[lemonbar] Started")

    while (true) {
        mutex.lock();
        s.stdin << outputs[0] << std::endl;
        DEB("Tick");

        char buffer[20] = "";
        s.stdout.readsome(buffer, 20);
        if (buffer[0] != '\0')
            DEB("AA" << buffer);
    }
    s.send_eof();
    s.wait();
}
