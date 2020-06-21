#include "../include/modules.h"
#include "../include/subprocess.h"

#include <thread>
#include<set>
#include <sstream>

bool modules::is_ok = true;

// TODO: Doesnt allow += or append..
void modules::update_function(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output,
                              const std::string &value) {
    std::shared_lock<std::shared_mutex> lock(data_mutex);
    output = value;
    wake_mutex.unlock();
}


struct Wrap {
    std::string prefix, suffix;

    Wrap &operator+=(const Wrap &w) {
        prefix.append(w.prefix);
        suffix.insert(0, w.suffix);

        return *this;
    }
};

Wrap operator+(const Wrap &w1, const Wrap &w2) {
    return {w1.prefix + w2.prefix, w2.suffix + w1.suffix};
}

std::string &operator+=(std::string &str, const Wrap &w) {
    str.insert(0, w.prefix);
    str.append(w.suffix);
    return str;
}

std::string operator+(const std::string &str, const Wrap &w) {
    return w.prefix + str + w.suffix;
}


// TODO: More helper functions like get_duration, get_color, etc.
// TODO: standardized log messages with entire message as one string ( no interleaving)

// Helpers:

// Set to prefix/suffix
void colorwrap(std::string &prefix, std::string &suffix,
               const std::string &color, const std::string &background = "") {
    // TODO: Use wraps
    if (!color.empty()) {
        prefix.insert(0, "%{F" + color + "}");
        suffix += "%{F-}" + suffix;
    }
    if (!background.empty()) {
        prefix.insert(0, "%{B" + background + "}");
        suffix += "%{B-}" + suffix;
    }
}

// Get wrap
Wrap colorreturn(const std::string &color, const std::string &background = "") {
    Wrap ret;
    colorwrap(ret.prefix, ret.suffix, color, background);
    return ret;
}

void actionwrap(std::string &prefix, std::string &suffix,
                const std::string &action, const std::string &buttons = "") {
    prefix.insert(0, "%{A" + buttons + ":" + action + ":}");
    suffix += "%{A}";
}

// Get wrap
Wrap actionreturn(const std::string &action, const std::string &buttons = "") {
    Wrap ret;
    actionwrap(ret.prefix, ret.suffix, action, buttons);
    return ret;
}

// Modules:
void modules::clock(modules::Updater update, const Options &options) {
    const size_t CLOCK_BUFFER_SIZE = 30;

    Wrap wrap = colorreturn(options.at("color"), options.at("background"));

    const char *format = options.at("format").data();
    char buffer[CLOCK_BUFFER_SIZE];
    std::time_t time;

    std::chrono::duration<int64_t> interval = std::chrono::seconds(1);
    try {
        // TODO: add float intervals -> milliseconds
        interval = std::chrono::seconds(std::stol(options.at("interval")));
    } catch (const std::exception &e) { // std::invalid_argument & std::out_of_range
        ERR("Invalid duration: " << options.at("interval"));
    }

    DEB("[clock@" << (size_t) &options % 1000 << "] " << "Started");

    while (modules::is_ok) {
        time = std::time(nullptr);
        if (!std::strftime(buffer, CLOCK_BUFFER_SIZE - 1, format, std::localtime(&time))) {
            ERR("[clock@" << (size_t) &options % 1000 << "] " << "Cant format time.");
            update("");
            return;
        }

        update(buffer + wrap);

        std::this_thread::sleep_for(interval);
    }
    DEB("[clock@" << (size_t) &options % 1000 << "] " << "Done");
}

void modules::text(const Updater update, const modules::Options &options) {
    Wrap wrap = colorreturn(options.at("color"), options.at("background"))
                + actionreturn(options.at("action"));

    DEB("[text@] " << "Started");

    update(options.at("text") + wrap);
}

void modules::left(const modules::Updater update, const modules::Options &options) {
    update("%{l}");
}

void modules::center(const modules::Updater update, const modules::Options &options) {
    update("%{c}");
}

void modules::right(const modules::Updater update, const modules::Options &options) {
    update("%{r}");
}

void modules::spacer(const modules::Updater update, const modules::Options &options) {
    update(options.at("spacer"));
}

// Uses systemd-networkd.service to find the status of the a wlan* and an enp* network.
// My laptop doesnt have ethernet
const char *const NETWORK_COMMAND[] = {"journalctl", "-fu", "systemd-networkd.service", "--no-pager", "-o", "cat"};

// Maybe use --no-tail?
// TODO: use power indicater
void modules::network(const modules::Updater update, const modules::Options &options) {
    Subprocess journal(NETWORK_COMMAND); // Use popopen?
    journal.send_eof(); // Close STDIN
    std::string line;

    Wrap wrap = colorreturn(options.at("color"), options.at("background"));

    bool enp_conn = false, wlan_conn = false, wlan_up = false;

    const bool show_ssid = options.at("show_ssid") == "true" || options.at("show_ssid") == "1";
    DEB(show_ssid);

    const std::string &wlan_conn_color = options.at("wlan_conn_color"),
            &wlan_up_color = options.at("wlan_up_color"), &wlan_down_color = options.at("wlan_down_color"),
            &enp_color = options.at("enp_color");

    const std::string &wlan_conn_sym = options.at("wlan_conn_sym"), &wlan_up_sym = options.at("wlan_up_sym"),
            &wlan_down_sym = options.at("wlan_down_sym"), &sep_sym = options.at("sep_sym"),
            &enp_sym = options.at("enp_sym");

    std::string wlan_ssid;
    const std::string &wlan_iface = options.at("wlan_iface");

    while (std::getline(journal.stdout, line)) {
        const size_t sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string_view iface = line, msg = line;
        iface.remove_suffix(line.length() - sep);
        msg.remove_prefix(sep + 2);

        if (iface.rfind(wlan_iface, 0) == 0) {
            if (msg == "Gained carrier") {
                wlan_conn = true;
            } else if (msg == "Lost carrier") {
                wlan_conn = false;
            } else if (msg == "Link UP") {
                wlan_up = true;
            } else if (msg == "Link DOWN") {
                wlan_up = false;
            } else if (msg.rfind("Connected WiFi access point: ", 0) == 0) {
                std::string_view sv(msg);
                sv.remove_prefix(29);
                sv.remove_suffix(sv.length() - sv.rfind('(') + 1);
                wlan_ssid = sv;
            }

        } else if (iface.rfind("enp", 0) == 0) {
            if (msg == "Gained carrier") {
                enp_conn = true;
            } else if (msg == "Lost carrier") {
                enp_conn = false;
            }
        } else {
            DEB("Unknown interface: " << iface);
        }


        if (!journal.stdout_buffer->in_avail()) { // Only updates before blocking io.
            std::string output = wrap.prefix;
            if (enp_conn) {
                output += (enp_sym + colorreturn(enp_color));
                output += sep_sym;
            }

            if (wlan_conn) {
                if (wlan_ssid.empty()) {
                    wlan_ssid = "unknown";
                    const char *const NETWORK_WPA_CLI_STATUS[] = {"wpa_cli", "-i",
                                                                  (const char *const) wlan_iface.data(), "status"};
                    Subprocess wpa_cli(NETWORK_WPA_CLI_STATUS);
                    wpa_cli.send_eof();
                    std::string wpa_line;
                    while (std::getline(wpa_cli.stdout, wpa_line)) {
                        if (wpa_line.rfind("ssid=", 0) == 0)
                            wlan_ssid = wpa_line.substr(5);//, wpa_line.length() - 5 + 1);
                    }
                    wpa_cli.wait();
                }

                output += ((wlan_conn_sym.empty() ? "" : wlan_conn_sym) +
                           (wlan_conn_sym.empty() && !show_ssid ? "" : " ") + (show_ssid ? wlan_ssid : "") +
                           colorreturn(wlan_conn_color));
            } else {
                output += ((wlan_up ? wlan_up_sym : wlan_down_sym) + colorreturn(wlan_down_color));
            }
            output += wrap.suffix;
            update(output);
        }
    }
}


// TODO: Check software
//const char *BATTERY_PATH = "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/PNP0C0A:03/power_supply/BAT0";
//const char *const BATTERY_COMMAND[] = {"udevadm", "monitor", "--subsystem-match=\"power_supply\"", "--udev"};
//const char *const BATTERY_INFO[] = {"udevadm", "info", "-p", "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/PNP0C0A:03/power_supply/BAT0"};

/*
void modules::battery(const modules::Updater update, const modules::Options &options) {
    INFO("[battery@]");
    // TODO: Why doesnt normal statement work?
    // Some stupid pointer problem.
    const std::vector<char const *> bat_cmd{"udevadm", "monitor", "--subsystem-match=\"power_supply\"", "--udev"};
    const std::vector<char const *> bat_info{"udevadm", "info", "-p", "/sys/class/power_supply/BAT0"};
//                                             "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/PNP0C0A:03/power_supply/BAT0"};

    Wrap wrap = colorreturn(options.at("color"), options.at("background"));


    const std::string &charge_sym = options.at("charge_sym");
    const std::string charge_spacer(charge_sym.length(), ' ');

    const std::string &chars = options.at("chars"), &chars_charging = options.at("chars_charging");

    // CANNOT USE `char` with symbol font. Need `
    std::vector<std::string> syms, syms_charging;
    size_t adv_len = 0;
    if (!chars.empty() && !chars_charging.empty()) {//&& (chars.size() == chars_charging.size())) {
        */
/*std::basic_istringstream<wchar_t> s(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(chars)),
                s_c(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(chars_charging));
        wchar_t c, c_c;
        while ((s >> c) && (s_c >> c_c)) {
            syms.push_back(c);
            syms_charging.push_back(c_c);
        }
        adv_len = syms.size();*//*

        const std::string delim = " ";
        auto start = 0U;
        auto end = chars.find(' ');
        while (end != std::string::npos) {
            syms.push_back(chars.substr(start, end - start));
            start = end + delim.length();
            end = chars.find(' ', start);
        }

        start = 0U;
        end = chars_charging.find(' ');
        while (end != std::string::npos) {
            syms_charging.push_back(chars_charging.substr(start, end - start));
            start = end + delim.length();
            end = chars_charging.find(' ', start);
        }

        adv_len = syms.size() > syms_charging.size() ? syms_charging.size() : syms.size();
    }
    auto basic = [&charge_sym, &charge_spacer](std::string &capacity, const bool charging) {
        return (charging ? charge_sym : charge_spacer) + (charge_sym.empty() ? "" : " ") + capacity + "%";
    };

    auto advanced = [&syms, &syms_charging, &adv_len](const std::string &capacity, const bool charging) {
        std::vector<std::string> &char_set = (charging ? syms_charging : syms);
        try {
            int cap = std::stoi(capacity);
            return char_set[(size_t) (adv_len * cap / 101)] + " " + capacity + "%";
        } catch (const std::invalid_argument &e) {
            return capacity + "%";

        }
    };

    //Subprocess udev_monitor(BATTERY_COMMAND);
    Subprocess udev_monitor(bat_cmd.data());
    udev_monitor.send_eof();

    std::string line;
    std::getline(udev_monitor.stdout, line); // monitor will print the received events for:
    std::getline(udev_monitor.stdout, line); // UDEV - the event which udev sends out after rule processing
    std::getline(udev_monitor.stdout, line); // "\n"

    // udevadm monitor sends three events for charging / discharging. TODO: One update
    do {
        if (!udev_monitor.stdout_buffer->in_avail()) { // Three updates have too big a gap.
            // const std::string BATTERY_PATH = line.substr(line.find('/'), line.rfind(" (power_supply)") - line.find('/')+ 1);
            // const char *const BATTERY_INFO[] = {"udevadm", "info", "-p", BATTERY_PATH};

            //Subprocess battery_info(BATTERY_INFO);
            Subprocess battery_info(bat_info.data());
            battery_info.send_eof();

            std::string battery_line, capacity;
            bool charging = false;
            while (std::getline(battery_info.stdout, battery_line)) {
                if (battery_line.rfind("E: POWER_SUPPLY_STATUS=", 0) == 0)
                    charging = battery_line.substr(23) == "Charging";
                else if (battery_line.rfind("E: POWER_SUPPLY_CAPACITY=", 0) == 0)
                    capacity = battery_line.substr(25);
            }
            if (adv_len) {
                update(advanced(capacity, charging) + wrap);
            } else {
                update(basic(capacity, charging) + wrap);
            }
            INFO("[battery] tick");
            battery_info.wait();
        }
    } while (std::getline(udev_monitor.stdout, line));
}
*/

void modules::battery(const modules::Updater update, const modules::Options &options) {
    INFO("[battery] Started");
    // TODO: Why doesnt normal statement work?
    // Some stupid pointer problem.
    const std::vector<char const *> bat_info{"udevadm", "info", "-p", "/sys/class/power_supply/BAT0"};
    const std::chrono::duration<int64_t> sleep_time = std::chrono::seconds(60);

    Wrap wrap = colorreturn(options.at("color"), options.at("background"));


    const std::string &charge_sym = options.at("charge_sym");
    const std::string charge_spacer(charge_sym.length(), ' ');

    const std::string &chars = options.at("chars"), &chars_charging = options.at("chars_charging");

    // CANNOT USE `char` with symbol font. Need `
    std::vector<std::string> syms, syms_charging;
    size_t adv_len = 0;
    if (!chars.empty() && !chars_charging.empty()) {//&& (chars.size() == chars_charging.size())) {
        /*std::basic_istringstream<wchar_t> s(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(chars)),
                s_c(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(chars_charging));
        wchar_t c, c_c;
        while ((s >> c) && (s_c >> c_c)) {
            syms.push_back(c);
            syms_charging.push_back(c_c);
        }
        adv_len = syms.size();*/
        const std::string delim = " ";
        auto start = 0U;
        auto end = chars.find(' ');
        while (end != std::string::npos) {
            syms.push_back(chars.substr(start, end - start));
            start = end + delim.length();
            end = chars.find(' ', start);
        }

        start = 0U;
        end = chars_charging.find(' ');
        while (end != std::string::npos) {
            syms_charging.push_back(chars_charging.substr(start, end - start));
            start = end + delim.length();
            end = chars_charging.find(' ', start);
        }

        adv_len = syms.size() > syms_charging.size() ? syms_charging.size() : syms.size();
    }
    auto basic = [&charge_sym, &charge_spacer](std::string &capacity, const bool charging) {
        return (charging ? charge_sym : charge_spacer) + (charge_sym.empty() ? "" : " ") + capacity + "%";
    };

    auto advanced = [&syms, &syms_charging, &adv_len](const std::string &capacity, const bool charging) {
        std::vector<std::string> &char_set = (charging ? syms_charging : syms);
        try {
            int cap = std::stoi(capacity);
            return char_set[(size_t) (adv_len * cap / 101)] + "" + capacity + "%";
        } catch (const std::invalid_argument &e) {
            return capacity + "%";

        }
    };

    // udevadm monitor sends three events for charging / discharging. TODO: One update
    while (modules::is_ok) {
        Subprocess battery_info(bat_info.data());
        battery_info.send_eof();

        std::string battery_line, capacity;
        bool charging = false;
        while (std::getline(battery_info.stdout, battery_line)) {
            if (battery_line.rfind("E: POWER_SUPPLY_STATUS=", 0) == 0)
                charging = battery_line.substr(23) == "Charging";
            else if (battery_line.rfind("E: POWER_SUPPLY_CAPACITY=", 0) == 0)
                capacity = battery_line.substr(25);
        }
        if (adv_len) {
            update(advanced(capacity, charging) + wrap);
        } else {
            update(basic(capacity, charging) + wrap);
        }
        //INFO("[battery] tick");
        battery_info.wait();
        std::this_thread::sleep_for(sleep_time);
    }
}

#include <i3ipc++/ipc.hpp>
#include <i3ipc++/ipc-util.hpp>
#include <sstream>

struct I3_wraps {
    Wrap ws_inactive, ws_active, title, mode;
};

void modules::i3(const modules::Updater update, const modules::Options &options) {
    INFO("[i3] started");

    size_t max_len;
    try {
        max_len = std::stoi(options.at("title_max_len"));
    } catch (const std::exception &e) {
        max_len = 10;
    }

    I3_wraps wraps{
            colorreturn(options.at("ws_inactive"), options.at("ws_inactive_bg")),
            colorreturn(options.at("ws_active"), options.at("ws_active_bg")),
            colorreturn(options.at("title"), options.at("title_bg")),
            colorreturn(options.at("mode"), options.at("mode_bg")),
    };

    const std::string mode_default = "default";
    const std::chrono::duration<int64_t> reload_sleep_time = std::chrono::seconds(1);

    std::set<std::pair<int, std::string>> workspaces;

    int insertion_num;
    std::string title, mode;

    auto print = [&update, &workspaces, &insertion_num, &title, &mode, &max_len, &mode_default, &wraps]() {
        std::stringstream ss;

        ss << wraps.ws_inactive.prefix;
        for (auto &ws : workspaces) {
            if (ws.first == insertion_num) {
                ss << wraps.ws_inactive.suffix << ws.second + wraps.ws_active;
                if (!title.empty())
                    ss << " " << title.substr(0, max_len) + wraps.title << " ";
                ss << wraps.ws_inactive.prefix;
            } else
                ss << ws.second;
            ss << " ";
        }
        ss << wraps.ws_inactive.suffix;

        if (mode != mode_default)
            ss << "  " << mode + wraps.mode;

        update(ss.str());
    };

    /*const std::map<i3ipc::WorkspaceEventType, std::string> ws_event_to_str{
        {i3ipc::WorkspaceEventType::FOCUS,    "FOCUS"},
        {i3ipc::WorkspaceEventType::EMPTY,    "EMPTY"},
        {i3ipc::WorkspaceEventType::INIT,     "INIT"},
        {i3ipc::WorkspaceEventType::URGENT,   "URGENT"},
        {i3ipc::WorkspaceEventType::RENAME,   "RENAME"},
        {i3ipc::WorkspaceEventType::RELOAD,   "RELOAD"},
        {i3ipc::WorkspaceEventType::RESTORED, "RESTORED"}
    };*/
    auto on_workspace = [/*&ws_event_to_str,*/ &workspaces, &insertion_num, &title, &print](
            const i3ipc::workspace_event_t &ev) {
        // INFO(ws_event_to_str.at(ev.type) << " " << ev.current->num);

        if (ev.type == i3ipc::WorkspaceEventType::FOCUS || ev.type == i3ipc::WorkspaceEventType::INIT) {
            insertion_num = ev.current->num;
            title = "";
        }

        if (ev.type == i3ipc::WorkspaceEventType::INIT)
            workspaces.insert({ev.current->num, ev.current->name});

        if (ev.type == i3ipc::WorkspaceEventType::EMPTY)
            workspaces.erase(workspaces.find({ev.current->num, ev.current->name}));

        // TODO
        // if (ev.type == i3ipc::WorkspaceEventType::{RELOAD, RENAMED, RESTORED})

        /* Probably useful later
         * auto it = std::find_if(st.begin(), st.end(), [](const pair<int,int>& p ){ return p.first == 1; });
         * if (it != st.end())
         * st.erase(it);
         */
        // TODO: Urgent
        print();
    };

    auto on_mode = [&mode, &print](const i3ipc::mode_t &mode_change) {
        mode = mode_change.change;
        print();
    };

    auto on_window = [&print, &title](const i3ipc::window_event_t &ev) {
        // INFO("[i3] " << ev.container->name << "___" << ev.container->window_properties.instance << "___" <<
        // INFO("[i3] " << ev.container->name << "___" << ev.container->window_properties.instance << "___" <<
        // ev.container->window_properties.xclass);

        title = ev.container->window_properties.xclass;
        print();
    };

    while (modules::is_ok) { // Reconnect when i3 reloads.
        try {
            i3ipc::connection conn;

            mode = mode_default;
            title = "";
            insertion_num = -1;

            /* Init workspaces */ {
                workspaces.clear();
                std::vector<std::shared_ptr<i3ipc::workspace_t>> workspace_objs = conn.get_workspaces();
                std::transform(workspace_objs.cbegin(), workspace_objs.cend(),
                               std::inserter(workspaces, workspaces.begin()),
                               [](const std::shared_ptr<i3ipc::workspace_t> &obj) {
                                   return std::make_pair(obj->num, obj->name);
                               });
            }

            print();

            conn.subscribe(i3ipc::EventType::ET_WORKSPACE | i3ipc::EventType::ET_WINDOW | i3ipc::EventType::ET_MODE);
            conn.signal_workspace_event.connect(on_workspace);
            conn.signal_mode_event.connect(on_mode);
            conn.signal_window_event.connect(on_window);

            while (modules::is_ok) conn.handle_event();

        } catch (const i3ipc::eof_error &e) {
            std::this_thread::sleep_for(reload_sleep_time);
            continue;
        }
        catch (const i3ipc::errno_error &e) {
            std::this_thread::sleep_for(reload_sleep_time);
            continue;
        }
        break;
    }
}

const char *const PULSEAUDIO_SUB[] = {"pactl", "subscribe"};
const char *const PULSEAUDIO_CHECK[] = {"pactl", "subscribe"};

void modules::pulseaudio(const modules::Updater update, const modules::Options &options) {

}

