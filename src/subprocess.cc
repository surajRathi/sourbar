#include "../include/subprocess.h"

#include "../include/log.h"

#include <unistd.h>
#include <sys/wait.h>

Pipe::Pipe() : fds{0, 0} {
    if (pipe(fds) == -1) {
        DEB("Could not create pipe.");
    }
}

Pipe::~Pipe() {
    close(fds[0]);
    close(fds[1]);
}

int Pipe::read_fd() {
    return fds[0];
}

int Pipe::write_fd() {
    return fds[1];
}

Subprocess::Subprocess(const char *const *argv /*= nullptr*/, const char *const *envp /*= nullptr*/) :
        pid(-1), stdin(nullptr), stdout(nullptr) {
    this->pid = fork();

    if (this->pid == -1) std::runtime_error("Couldn't Fork");
    else if (this->pid == 0) {
        close(read_pipe.write_fd());
        close(write_pipe.read_fd());
        dup2(read_pipe.read_fd(), STDIN_FILENO);
        dup2(write_pipe.write_fd(), STDOUT_FILENO);

        if (envp != nullptr)
            execve(argv[0], const_cast<char *const *>(argv), const_cast<char *const *>(envp));
        else execvp(argv[0], const_cast<char *const *>(argv));
        exit(1);
    } else {
        close(write_pipe.write_fd());
        close(read_pipe.read_fd());

        stdin_buffer = std::make_unique<__gnu_cxx::stdio_filebuf<char> >(read_pipe.write_fd(), std::ios::out);
        stdout_buffer = std::make_unique<__gnu_cxx::stdio_filebuf<char> >(write_pipe.read_fd(), std::ios::in);

        stdin.rdbuf(stdin_buffer.get());
        stdout.rdbuf(stdout_buffer.get());
    }
}

void Subprocess::send_eof() const {
    stdin_buffer->close();
}

int Subprocess::wait() const {
    int status;
    waitpid(pid, &status, 0);
    return status;
}
