//
// Created by suraj on 5/6/20.
//

#ifndef BAR_MODULES_H
#define BAR_MODULES_H

#include <map>
#include <string>

#include <mutex>
#include "../include/log.h"

/* Each module requires an entry in the module_map */

namespace modules {
    typedef void (*Module)(std::mutex &, std::string &, const std::map<std::string, std::string> &);

    typedef std::map<std::string, std::string> Options;

    typedef const std::map<std::string, std::pair<Module, Options>> ModuleMap;


    const Options clock_options = {
            {"color",    "#FFFFFF"},
            {"interval", "1"}
    };

    [[noreturn]] void clock(std::mutex &mutex, std::string &t, const Options &options);


    ModuleMap module_map = {
            {"clock", std::make_pair(&clock, clock_options)}
    };
}

#endif //BAR_MODULES_H

