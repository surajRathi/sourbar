#include "../include/log.h"

#include "../include/subprocess.h"

const char *const command[] = {"bash", "-c", "ls"};

int main() {
    LOG("Started");

    // Parse Config

    // Start module threads

    // Use https://gist.github.com/konstantint/d49ab683b978b3d74172
    Subprocess s(command);
    s.send_eof();

    LOG("Reading stdout: ");
    char buf[32];
    while (s.stdout) {
        s.stdout >> buf;
        std::cout << buf << std::endl;
    }
    s.wait();

    LOG("Finished");
    return 0;
}
