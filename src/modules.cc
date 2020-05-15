#include "../include/modules.h"
#include "../include/subprocess.h"

#include <thread>

bool modules::is_ok = true;

// TODO: Doesnt allow += or append..
void modules::update_function(std::mutex &wake_mutex, std::shared_mutex &data_mutex, std::string &output,
                              const std::string &value) {
    std::shared_lock<std::shared_mutex> lock(data_mutex);
    output = value;
    wake_mutex.unlock();
}

// TODO: More helper functions like get_duration, get_color, etc.
// TODO: standardized log messages with entire message as one string ( no interleaving)

// Helpers:
void colorwrap(std::string &prefix, std::string &suffix,
               const modules::Options &options) {
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
    } catch (const std::out_of_range &e) {
    }
}

void colorwrap(std::string &str, const std::string &color = "", const std::string &background = "") {
    str = (color.empty() ? "" : "%{F:" + color + "}") +
          (background.empty() ? "" : "%{F:" + background + "}") + str + (background.empty() ? "" : "%{B-}") +
          (color.empty() ? "" : "%{F-}");
}

std::string colorreturn(const std::string &str, const std::string &color = "", const std::string &background = "") {
    std::string ret = str;
    colorwrap(ret, color, background);
    return ret;
}

std::string colorreturn(std::string &str, const std::string &color = "", const std::string &background = "") {
    colorwrap(str, color, background);
    return str;
}


void actionwrap(std::string &prefix, std::string &suffix,
                const std::string &action, const std::string &buttons = "") {
    prefix += "%{A" + buttons + ":" + action + ":}";
    suffix += "%{A}";
}

void actionwrap(std::string &prefix, std::string &suffix, const modules::Options &options) {
    auto iter = options.find("action");
    if (iter != options.end() && !iter->second.empty())
        actionwrap(prefix, suffix, iter->second);
}

// Modules:
const size_t CLOCK_BUFFER_SIZE = 30;

void modules::clock(modules::Updater update, const Options &options) {
    std::string prefix, suffix;
    colorwrap(prefix, suffix, options);

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

        update(prefix + buffer + suffix);

        std::this_thread::sleep_for(interval);
    }
    DEB("[clock@" << (size_t) &options % 1000 << "] " << "Done");
}

void modules::text(const Updater update, const modules::Options &options) {
    std::string prefix, suffix;
    colorwrap(prefix, suffix, options);
    actionwrap(prefix, suffix, options);

    DEB("[text@" << (size_t) &options % 1000 << "] " << "Started");

    update(prefix + options.at("text") + suffix);

    DEB("[text@" << (size_t) &options % 1000 << "] " << "Done");
}

void modules::left(const modules::Updater update, const modules::Options &options) {
    update("%l}");
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

    std::string prefix, suffix;
    colorwrap(prefix, suffix, options);

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
    const char *const wlan_iface = "wlan0";

    while (std::getline(journal.stdout, line)) {
        const size_t sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string_view iface = line, msg = line;
        iface.remove_suffix(line.length() - sep);
        msg.remove_prefix(sep + 2);

        if (iface.rfind("wlan", 0) == 0) {
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
            std::string output = prefix;
            if (enp_conn) {
                output += colorreturn(enp_sym, enp_color);
                output += sep_sym;
            }

            if (wlan_conn) {
                if (wlan_ssid.empty()) {
                    wlan_ssid = "unknown";
                    const char *const NETWORK_WPA_CLI_STATUS[] = {"wpa_cli", "-i", wlan_iface, "status"};
                    Subprocess wpa_cli(NETWORK_WPA_CLI_STATUS);
                    wpa_cli.send_eof();
                    std::string wpa_line;
                    while (std::getline(wpa_cli.stdout, wpa_line)) {
                        if (wpa_line.rfind("ssid=", 0) == 0)
                            wlan_ssid = wpa_line.substr(5);//, wpa_line.length() - 5 + 1);
                    }
                    wpa_cli.wait();
                }

                output += colorreturn((wlan_conn_sym.empty() ? "" : wlan_conn_sym) +
                                      (wlan_conn_sym.empty() && !show_ssid ? "" : " ") + (show_ssid ? wlan_ssid : ""),
                                      wlan_conn_color);
            } else {
                output += colorreturn(wlan_up ? wlan_up_sym : wlan_down_sym, wlan_down_color);
            }
            output += suffix;
            update(output);
        }
    }
}

const char *const BATTERY_COMMAND[] = {"udevadm", "monitor", " --subsystem-match=\"power_supply\"", "--udev"};
// TODO: Check software
//const char *BATTERY_PATH = "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/PNP0C0A:03/power_supply/BAT0";
//const char *const BATTERY_INFO[] = {"udevadm", "info", "-p", BATTERY_PATH};
const char *const BATTERY_INFO[] = {"udevadm", "info", "-p",
                                    "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/PNP0C0A:03/power_supply/BAT0"};

void modules::battery(const modules::Updater update, const modules::Options &options) {
    INFO("[battery@]");
    std::string prefix, suffix;
    colorwrap(prefix, suffix, options);

    const std::string &charge_sym = options.at("charge_sym");
    const std::string charge_spacer(charge_sym.length(), ' ');

    Subprocess udev_monitor(BATTERY_COMMAND);
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
            Subprocess battery_info(BATTERY_INFO);
            battery_info.send_eof();

            std::string battery_line, status, capacity;
            while (std::getline(battery_info.stdout, battery_line)) {
                if (battery_line.rfind("E: POWER_SUPPLY_STATUS=", 0) == 0)
                    status = battery_line.substr(23);
                else if (battery_line.rfind("E: POWER_SUPPLY_CAPACITY=", 0) == 0)
                    capacity = battery_line.substr(25);
            }
            update(prefix + (status == "Charging" ? charge_sym : charge_spacer) +
                   (capacity.length() == 1 ? " " : "") + capacity +
                   "%" + suffix);
        }
    } while (std::getline(udev_monitor.stdout, line));
}


