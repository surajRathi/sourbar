#include "../include/log.h"

#include "../include/subprocess.h"
#include "../include/modules.h"


#include <thread>
#include <functional>
#include <mutex>

const char *const lemon_cmd[] = {"lemonbar", "-f", "-misc-dejavu sans-medium-r-normal--0-100-0-0-p-9-ascii-0", "-f",
                                 "-wuncon-siji-medium-r-normal--0-0-75-75-c-0-iso10646-1", "-F", "#F5F6F7", "-B",
                                 "#303642"};

// Converting to vector and/or maps will probably allow runtime configeration.

module::Module modules[] = {&module::time};

const size_t num_modules = 1;

const std::array<std::string, num_modules> outputs;
const std::thread *threads[num_modules];

const auto TIME = outputs[0];


int main() {
    LOG("Started");

    std::mutex mutex;

    for (int i = 0; i < num_modules; i++)
        threads[i] = new std::thread(modules[i], std::ref(mutex), std::ref(const_cast<std::string &>(outputs[i])));

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