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

const char *const NETWORK_COMMAND[] = {"journalctl", "-fu", "systemd-networkd.service", "--no-pager", "-o", "cat"};
// Maybe use --no-tail?

void modules::network(const modules::Updater update, const modules::Options &options) {
    Subprocess journal(NETWORK_COMMAND); // TODO: Use popopen
    journal.send_eof();

    std::string line, wlan, enp;

    bool wlan_conn = false, wlan_up = false;
    const std::string wlan_conn_color, wlan_up_color, wlan_down_color;
    const std::string wlan_up_sym = "?", sep_sym = " | ", wlan_conn_sym = "d";
    std::string wlan_ssid;
    const std::string wlan_down_sym = "!";

    bool enp_conn = false;
    const std::string enp_color, enp_sym = ">";

    while (std::getline(journal.stdout, line)) {
        const size_t sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string_view iface = line, msg = line;
        iface.remove_suffix(line.length() - sep);
        msg.remove_prefix(sep + 2);

        if (iface.rfind("wlan", 0) == 0) {
            if (msg == "Gained carrier")
                wlan_conn = true;
            else if (msg == "Lost carrier") {
                wlan_conn = false;
                if (!wlan_ssid.empty())
                    INFO("Disconnected from " << wlan_ssid);
                wlan_ssid = "";
            } else if (msg == "Link UP") {
                wlan_up = true;
                INFO("Wifi Up.");
            } else if (msg == "Link DOWN") {
                wlan_up = false;
                INFO("Wifi Down.");
            } else if (msg.rfind("Connected WiFi access point: ", 0) == 0) {
                std::string_view sv(msg);
                sv.remove_prefix(29);
                sv.remove_suffix(sv.length() - sv.rfind('(') + 1);
                wlan_ssid = sv;
                INFO("Connected to " << sv << ".");
            }

        } else if (iface.rfind("enp", 0) == 0) {
            if (msg == "Gained carrier") {
                enp_conn = true;
                INFO("USB tether connected");
            } else if (msg == "Lost carrier") {
                enp_conn = false;
                INFO("USB tether disconnected.");
            }
        } else {
            DEB("Unknown interface: " << iface);
        }

        if (!journal.stdout_buffer->in_avail()) { // Only updates before blocking io.
            std::string output;
            if (enp_conn) {
                output += colorreturn(enp_sym, enp_color);
                output += sep_sym;
            }

            if (wlan_conn) {
                output += wlan_conn_sym;
                output += " ";
                output += colorreturn(wlan_ssid, wlan_conn_color);
            } else if (wlan_up) {
                output += colorreturn(wlan_up_sym, wlan_up_color);
            } else {
                output += colorreturn(wlan_down_sym, wlan_down_color);
            }
            update(output);
        }
    }

}