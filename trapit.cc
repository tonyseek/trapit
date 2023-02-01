// Copyright Jiangge Zhang (License: MIT)

#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>

#ifndef TRAPIT_MAX_ARGS
#define TRAPIT_MAX_ARGS 4096
#endif

#ifndef TRAPIT_PORT
#define TRAPIT_PORT 26842
#endif

int cmd_usage(const char *prog) noexcept;
int cmd_trap(const char *prog, int argc, char **argv) noexcept;
int cmd_wake(const char *prog, int argc, char **argv) noexcept;

enum class DiscoverMode { server, client };

class Discover {
 private:
  int fd;
  in_port_t port;
  DiscoverMode mode;

 public:
  explicit Discover(in_port_t port, DiscoverMode mode) noexcept;
  ~Discover();
  Discover(const Discover &) = delete;             // non-copyable
  Discover &operator=(const Discover &) = delete;  // non-copyable
  int run() noexcept;
};

int main(int argc, char **argv) {
    int _argc = argc - 2;
    char **_argv = argv + 2;

    if (_argc < 0) {
        return cmd_usage(argv[0]);
    } else if (strcmp(argv[1], "exec") == 0) {
        if (_argc > 0 && strcmp(_argv[0], "--") == 0) {
            _argc--;
            _argv++;
        }
        return cmd_trap(argv[0], _argc, _argv);
    } else if (strcmp(argv[1], "wake") == 0) {
        return cmd_wake(argv[0], _argc, _argv);
    } else if (strcmp(argv[1], "help") == 0) {
        cmd_usage(argv[0]);
        return 0;
    } else {
        return cmd_usage(argv[0]);
    }
    return 0;
}

int cmd_usage(const char *prog) noexcept {
    const char *hl = "Usage: ";
    const char *pr = "       ";
    std::cerr << hl << prog << " [exec|wake|help]" << std::endl;
    std::cerr << pr << prog << " exec -- [argument ...]" << std::endl;
    std::cerr << pr << prog << " wake" << std::endl;
    return 2;
}

int cmd_trap(const char *prog, int argc, char **argv) noexcept {
    /*
     * Layout: [ARG 0] [ARG 0] [ARG 1] ... [NULL]
     */
    const char *exec_argv[TRAPIT_MAX_ARGS];

    if (argc <= 0) {
        return cmd_usage(prog);
    } else if (argc >= TRAPIT_MAX_ARGS - 1) {
        std::cerr << "Up to " << TRAPIT_MAX_ARGS - 2
            << " arguments, got " << argc << " arguments";
        return 1;
    } else {
        exec_argv[0] = argv[0];
        for (int i = 0; i < argc; i++) {
            exec_argv[i + 1] = argv[i];
        }
        exec_argv[argc + 1] = NULL;
    }

    {
        Discover d(TRAPIT_PORT, DiscoverMode::server);
        if (d.run() < 0) {
            return 1;
        }
    }

    execvp(exec_argv[0], (char *const *) exec_argv);
    std::cerr << "Cannot execute " << exec_argv[0] << ": " <<
        strerror(errno) << " (errno=" << errno << ")" << std::endl;
    return 1;
}

int cmd_wake(const char *prog, int argc, char **argv) noexcept {
    std::cout << "Work-in-Progress: wake" << std::endl;
    return 0;
}

Discover::Discover(in_port_t port, DiscoverMode mode) noexcept
    : fd(-1), port(port), mode(mode) {}

Discover::~Discover() {
    if (fd > 0) {
        close(fd);
    }
}

int Discover::run() noexcept {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "Cannot create socket FD: " << strerror(errno) <<
            " (errno=" << errno << ")" << std::endl;
        return fd;
    }

    struct sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {
            .s_addr = inet_addr("127.0.0.1"),
        },
    };

    switch (mode) {
        case DiscoverMode::server:
            if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
                std::cerr << "Cannot bind " << fd << "on 127.0.0.1:" << port <<
                    ": " << strerror(errno) << " (errno=" << errno << ")" <<
                    std::endl;
                return -1;
            }
            while (1) {
                char cdata = '\0';
                ssize_t csize;
                struct sockaddr_in caddr;
                socklen_t ncaddr;
                csize = recvfrom(
                    fd, &cdata, sizeof(cdata), 0, (struct sockaddr *) &caddr,
                    &ncaddr);
                if (csize < 0) {
                    std::cerr << "Cannot recvfrom " << fd << ":" <<
                        strerror(errno) << " (errno=" << errno << ")" <<
                        std::endl;
                    return csize;
                }
                if (ncaddr == 0) {
                    ncaddr = sizeof(caddr);  // Fix compatibility of macOS
                }

                pid_t pid;
                uint32_t pidnl;
                char pidbuf[sizeof(pidnl)];

                if (csize == sizeof(cdata) && cdata == '\n') {
                    pid = getpid();
                } else {
                    pid = 0;
                }

                pidnl = htonl(pid);
                memcpy(pidbuf, &pidnl, sizeof(pidnl));

                ssize_t nsent = sendto(
                    fd, pidbuf, sizeof(pidbuf), 0, (struct sockaddr *) &caddr,
                    ncaddr);
                if (nsent != sizeof(pidbuf)) {
                    continue;
                }

                if (pid > 0) {
                    return 0;
                } else {
                    continue;
                }
            }
            break;
        case DiscoverMode::client:
            break;
    }

    return 0;
}
