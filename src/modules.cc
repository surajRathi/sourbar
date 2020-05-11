#include "../include/modules.h"
#include "../include/subprocess.h"

#include <thread>

#include <ctime>
#include <shared_mutex>

bool modules::is_ok = true;

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

void
modules::clock(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output, const Options &options) {
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
            std::shared_lock<std::shared_mutex> lock(data_mutex);
            output = "";
            return;
        }

        {
            std::shared_lock<std::shared_mutex> lock(data_mutex);
            output = prefix + buffer + suffix;
        }

        wake_mutex.unlock();
        std::this_thread::sleep_for(interval);
    }
    INFO("[clock@" << (size_t) &options % 1000 << "] " << "Done");
}


void
modules::lemonbar(std::mutex &wake_mutex, std::shared_mutex &data_mutex,
                  std::vector<std::unique_ptr<std::string>> &outputs,
                  const Options &options) {
    const char *const lemon_cmd[] = {
            "lemonbar",
            "-n", options.at("name").data(),
            "-f", options.at("font-1").data(),
            "-f", options.at("font-2").data(),
            "-F", options.at("color").data(),
            "-B", options.at("background").data()
    };


    bool force_sleep;
    auto force_sleep_interval = std::chrono::milliseconds(0);
    try {
        auto iter = options.find("force_sleep_interval");
        force_sleep_interval = std::chrono::milliseconds(
                (int) (std::stof(iter->second) / 1000));
        force_sleep = true;
    } catch (const std::exception &e) {
        force_sleep = false;
    }
    Subprocess s(lemon_cmd);

    INFO("[lemonbar] Started");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    INFO("[lemonbar] Running");


    while (modules::is_ok) {
        wake_mutex.lock(); // This mutex should only be unlocked by other threads when new output is availible.

        {
            std::unique_lock<std::shared_mutex> lock(data_mutex);
            for (auto &output : outputs)
                s.stdin << *output;
        }
        s.stdin << std::endl;
        DEB("tick");

        char buffer[20] = "";
        s.stdout.readsome(buffer, 20);
        if (buffer[0] != '\0')
            DEB("Lemonbar gave: " << buffer);

        if (force_sleep)
            std::this_thread::sleep_for(force_sleep_interval);
    }
    s.send_eof();
    s.wait();
}

void
modules::text(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output,
              const modules::Options &options) {
    const auto&[prefix, suffix] = colorwrap(options);

    INFO("[text@" << (size_t) &options % 1000 << "] " << "Started");
    {
        std::shared_lock<std::shared_mutex> lock(data_mutex);
        output = prefix + options.at("text") + suffix;
    }

    wake_mutex.unlock();
    INFO("[text@" << (size_t) &options % 1000 << "] " << "Done");
}
