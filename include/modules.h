//
// Created by suraj on 5/6/20.
//

#ifndef BAR_MODULES_H
#define BAR_MODULES_H

#include <map>
#include <string>

#include <mutex>
#include "../include/log.h"


namespace module {
    typedef void (*Module)(std::mutex &, std::string &, const std::map<std::string, std::string> &);

    typedef std::map<std::string, std::string> OptionsMap;

    typedef const std::map<std::string, std::pair<Module, OptionsMap>> ModuleMap;

    const OptionsMap time_opts = {
            {"color",    "#FFFFFF"},
            {"interval", "1"}
    };

    [[noreturn]] void time(std::mutex &mutex, std::string &t, const OptionsMap &options);


    ModuleMap module_map = {
            {"time", std::make_pair(&time, time_opts)}
    };
}

#endif //BAR_MODULES_H

