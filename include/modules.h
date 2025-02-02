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


    typedef void (*Module)(Updater u, const Options &);

    typedef void (*Bar)(std::mutex &, std::shared_mutex &, std::vector<std::unique_ptr<std::string>> &,
                        const Options &);

    typedef const std::map<std::string, std::pair<Module, Options>> ModuleMap;
    typedef const std::map<std::string, Bar> BarMap;

    const Options empty_options{};

    const Options lemonbar_options{
            {"name",                 "lemonbar"},
            {"color",                "#FFFFFF"},
            {"background",           "#000000"},
            {"font-1",               "DejaVu"},
            {"font-2",               ""},

            // Convert to a single option following lemonbar spec?
            {"width",                ""},
            {"height",               "25"},
            {"x",                    ""},
            {"y",                    ""},

            {"force_sleep_interval", ""}
    };

    void lemonbar(std::mutex &wake_mutex, std::shared_mutex &data_mutex,
                  std::vector<std::unique_ptr<std::string>> &outputs,
                  const Options &options); // In modules_lemonbar.cc


    const Options text_options{
            {"color",      ""},
            {"background", ""},
            {"text",       ""},
            {"action",     ""}
    };

    void text(const Updater update, const Options &options);

    const Options clock_options{
            {"color",      ""},
            {"background", ""},
            {"interval",   "10"},
            {"format",     "%l:%M.%S %p"}
    };

    //void clock(std::mutex &mutex, std::shared_mutex &, std::string &output, const Options &optionsclock_options);
    void clock(const Updater update, const Options &options);

    void left(const Updater update, const Options &options);

    void center(const Updater update, const Options &options);

    void right(const Updater update, const Options &options);

    const Options spacer_options{
            {"spacer", " "}
    };

    void spacer(const Updater update, const Options &options);

    const Options network_options{
            {"wlan_iface",      "wlp1s0"},
            {"color",           ""},
            {"background",      ""},
            {"show_ssid",       "true"},
            {"wlan_conn_sym",   ""},
            {"wlan_up_sym",     "<"},
            {"wlan_down_sym",   "x"},
            {"sep_sym",         " | "},
            {"enp_sym",         ">"},
            {"wlan_conn_color", ""},
            {"wlan_up_color",   ""},
            {"wlan_down_color", ""},
            {"enp_color",       ""}
    };

    void network(const Updater update, const Options &options);

    const Options battery_options{
            {"interval",       "10"},
            {"color",          ""},
            {"background",     ""},
            {"charge_sym",     "~"},
            {"chars",          ""},
            {"chars_charging", ""}
    };

    void battery(const Updater update, const Options &options);

    const Options pulseaudio_options{
            {"color",           ""},
            {"background",      ""},
            {"vol_syms",        "~"},
            {"volume_mute_sym", ""}
    };

    void pulseaudio(const Updater update, const Options &options);

    const Options i3_options{
            {"color",          ""},
            {"background",     ""},
            {"title_max_len",  ""},

            {"ws_inactive",    ""},
            {"ws_inactive_bg", ""},
            {"ws_active",      ""},
            {"ws_active_bg",   ""},

            {"title",          ""},
            {"title_bg",       ""},

            {"mode",           ""},
            {"mode_bg",        ""}
    };

    void i3(const Updater update, const Options &options);


    ModuleMap module_map{
            {"clock",    std::make_pair(&clock, clock_options)},

            {"text",     std::make_pair(&text, text_options)},
            {"left",     std::make_pair(&left, empty_options)},
            {"center",   std::make_pair(&center, empty_options)},
            {"right",    std::make_pair(&right, empty_options)},
            {"spacer",   std::make_pair(&spacer, spacer_options)},

            {"network",  std::make_pair(&network, network_options)},
            {"battery",  std::make_pair(&battery, battery_options)},
            {"i3",       std::make_pair(&i3, i3_options)},

            {"lemonbar", std::make_pair(nullptr, lemonbar_options)}
    };

    BarMap bars{
            {"lemonbar", &lemonbar}
    };

    void update_function(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output,
                         const std::string &value);
}

#endif //BAR_MODULES_H

