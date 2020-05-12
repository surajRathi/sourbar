#ifndef BAR_MODULES_H
#define BAR_MODULES_H

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <shared_mutex>
#include <memory>
#include <functional>
#include "../include/log.h"

/* Each module requires an entry in the module_map */

// TODO: don't initialize string for each map key. use string_view?
// Changing an option name will be hell. enum with Lookup table?
// TODO: Encapsulate the data mutex and setting of output.

namespace modules {
    extern bool is_ok;

    typedef std::map<const std::string, std::string> Options;


    typedef std::_Bind_helper<0, void (&)(std::mutex &, std::shared_mutex &, std::string &, const std::string &),
            std::reference_wrapper<std::mutex>, std::reference_wrapper<std::shared_mutex>,
            std::reference_wrapper<std::string>, const std::_Placeholder<1> &>::type
            Updater;

    //typedef void (*Module)(std::mutex &, std::shared_mutex &, std::string &, const Options &);
    typedef void (*Module)(Updater u, const Options &);

    typedef void (*Bar)(std::mutex &, std::shared_mutex &, std::vector<std::unique_ptr<std::string>> &,
                        const Options &);

    typedef const std::map<std::string, std::pair<Module, Options>> ModuleMap;
    typedef const std::map<std::string, Bar> BarMap;


    const Options lemonbar_options = {
            {"name",                 "lemonbar"},
            {"color",                "#FFFFFF"},
            {"background",           "#000000"},
            {"font-1",               "-misc-dejavu sans-medium-r-normal--0-100-0-0-p-9-ascii-0"},
            {"font-2",               "-wuncon-siji-medium-r-normal--0-0-75-75-c-0-iso10646-1"},
            {"force_sleep_interval", ""}
    };

    void lemonbar(std::mutex &wake_mutex, std::shared_mutex &data_mutex,
                  std::vector<std::unique_ptr<std::string>> &outputs,
                  const Options &options = lemonbar_options); // In modules_lemonbar.cc


    const Options text_options = {
            {"color",      ""},
            {"background", ""},
            {"text",       ""}
    };

    void text(const Updater update, const Options &options = text_options);

    const Options clock_options = {
            {"color",      ""},
            {"background", ""},
            {"interval",   "10"},
            {"format",     "%l:%M.%S %p"}
    };

    //void clock(std::mutex &mutex, std::shared_mutex &, std::string &output, const Options &options = clock_options);
    void clock(const Updater update, const Options &options = clock_options);


    ModuleMap module_map = {
            {"clock",    std::make_pair(&clock, clock_options)},
            {"text",     std::make_pair(&text, text_options)},
            {"lemonbar", std::make_pair(nullptr, lemonbar_options)}
    };

    BarMap bars = {
            {"lemonbar", &lemonbar}
    };

    void update_function(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output,
                         const std::string &value);
}

#endif //BAR_MODULES_H

