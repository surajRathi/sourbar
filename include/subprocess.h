#ifndef BAR_SUBPROCESS_H
#define BAR_SUBPROCESS_H

#include <memory>
#include <ext/stdio_filebuf.h>

class Pipe {
    int fds[2];
public:
    Pipe();

    ~Pipe();

    int read_fd();

    int write_fd();
};


class Subprocess {
    Pipe read_pipe, write_pipe; // In child process terminology.

public:
    int pid;

    std::unique_ptr<__gnu_cxx::stdio_filebuf<char>> stdin_buffer;
    std::unique_ptr<__gnu_cxx::stdio_filebuf<char>> stdout_buffer;

    std::ostream stdin;
    std::istream stdout;

    Subprocess(const char *const *argv, const char *const *envp = nullptr);

    void send_eof() const;

    int wait() const;
};


#endif //BAR_SUBPROCESS_H
