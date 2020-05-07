#include "../include/modules.h"

#include <thread>
#include <mutex>

#include <ctime>

const size_t buffer_size = 20;

[[noreturn]] void modules::clock(std::mutex &mutex, std::string &t, const Options &options) {
    LOG("Time module started.");
    char buffer[buffer_size];
    char format[] = "%H:%M:%S";

    std::time_t time;
    auto one_second = std::chrono::seconds(1);

    while (true) {
        time = std::time(nullptr);
        if (!std::strftime(buffer, buffer_size, format, std::localtime(&time)))
            continue;
        t = buffer;

        mutex.unlock();
        std::this_thread::sleep_for(one_second);
    }
}