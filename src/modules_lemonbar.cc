#include "../include/modules.h"
#include "../include/subprocess.h"

#include <thread>

void lemonbar_output_handler(std::istream &stdout) {
    std::string line;
    while (std::getline(stdout, line)) {
        INFO("Lemonbar -> " << line);
    }
}

void modules::lemonbar(std::mutex &wake_mutex, std::shared_mutex &data_mutex,
                       std::vector<std::unique_ptr<std::string>> &outputs, const Options &options) {
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
        const auto iter = options.find("force_sleep_interval");
        force_sleep_interval = std::chrono::milliseconds(
                (int) (std::stof(iter->second) / 1000));
        force_sleep = true;
    } catch (const std::exception &e) { // from std::stof
        force_sleep = false;
    }
    Subprocess s(lemon_cmd);
    std::thread output_handler(lemonbar_output_handler, std::ref(s.stdout));
    INFO("[lemonbar] Started");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (modules::is_ok) {
        wake_mutex.lock(); // This mutex should be unlocked by other threads when new output is availible.
        {
            std::unique_lock<std::shared_mutex> lock(data_mutex);
            for (auto &output : outputs)
                s.stdin << *output;
            s.stdin << std::endl;
        }

        //DEB("tick");

        if (force_sleep)
            std::this_thread::sleep_for(force_sleep_interval);
    }
    s.send_eof();
    output_handler.join();
    s.wait();
}

