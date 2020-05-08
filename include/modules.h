#ifndef BAR_MODULES_H
#define BAR_MODULES_H

#include <map>
#include <string>
#include <vector>

#include <mutex>
#include "../include/log.h"

/* Each module requires an entry in the module_map */

namespace modules {

    typedef std::map<std::string, std::string> Options;

    typedef void (*Module)(std::mutex &, std::string &, const Options);

    typedef void (*Bar)(std::mutex &, std::vector<std::string> &, const Options);

    typedef const std::map<std::string, std::pair<Module, Options>> ModuleMap;
    typedef const std::map<std::string, Bar> BarMap;


    const Options lemonbar_options = {
            {"name",       "lemonbar"},
            {"color",      "#F5F6F7"},
            {"background", "#90303642"},
            {"font-1",     "-misc-dejavu sans-medium-r-normal--0-100-0-0-p-9-ascii-0"},
            {"font-2",     "-wuncon-siji-medium-r-normal--0-0-75-75-c-0-iso10646-1"}
    };

    [[noreturn]] void
    lemonbar(std::mutex &mutex, std::vector<std::string> &outputs, const Options options = lemonbar_options);

    // Changing an option name will be hell. enum with switch?
    const Options clock_options = {
            {"color",      ""},
            {"background", ""},
            {"interval",   "1"},
            {"format",     "%l:%M.%S %p"}
    };

    [[noreturn]] void clock(std::mutex &mutex, std::string &output, const Options options = clock_options);


    ModuleMap module_map = {
            {"clock",    std::make_pair(&clock, clock_options)},
            {"lemonbar", std::make_pair(nullptr, lemonbar_options)}
    };

    BarMap bars = {
            {"lemonbar", &lemonbar}
    };
}

#endif //BAR_MODULES_H

