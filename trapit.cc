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

#ifndef TRAPIT_VERSION
#define TRAPIT_VERSION "unstable"
#endif

const std::string version(TRAPIT_VERSION);

int cmd_usage(const char *prog) noexcept;
int cmd_version(const char *prog) noexcept;
int cmd_trap(const char *prog, int argc, char **argv) noexcept;
int cmd_wake(const char *prog, int argc, char **argv) noexcept;

enum class DiscoverMode { server, client };

class Discover {
 private:
  int m_fd;
  in_port_t m_port;
  DiscoverMode m_mode;
  pid_t m_pid;
  int set_timeout(uint32_t) noexcept;

 public:
  explicit Discover(in_port_t, DiscoverMode) noexcept;
  ~Discover();
  Discover(const Discover &) = delete;             // non-copyable
  Discover &operator=(const Discover &) = delete;  // non-copyable
  int run() noexcept;
  pid_t pid() const noexcept;
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
    } else if (strcmp(argv[1], "version") == 0) {
        return cmd_version(argv[0]);
    } else {
        return cmd_usage(argv[0]);
    }
    return 0;
}

int cmd_usage(const char *prog) noexcept {
    const char *hl = "Usage: ";
    const char *pr = "       ";
    std::cerr << hl << prog << " [exec|wake|version|help]" << std::endl;
    std::cerr << pr << prog << " exec -- [argument ...]" << std::endl;
    std::cerr << pr << prog << " wake" << std::endl;
    return 2;
}

int cmd_version(const char *prog) noexcept {
    std::cout << version << std::endl;
    return 0;
}

int cmd_trap(const char *prog, int argc, char **argv) noexcept {
    /*
     * Layout: [ARG 0] [ARG 1] ... [NULL]
     */
    const char *exec_argv[TRAPIT_MAX_ARGS];

    if (argc <= 0) {
        return cmd_usage(prog);
    } else if (argc >= TRAPIT_MAX_ARGS - 1) {
        std::cerr << "Up to " << TRAPIT_MAX_ARGS - 2
            << " arguments, got " << argc << " arguments";
        return 1;
    } else {
        for (int i = 0; i < argc; i++) {
            exec_argv[i] = argv[i];
        }
        exec_argv[argc + 1] = NULL;
    }

    {
        Discover d(TRAPIT_PORT, DiscoverMode::server);
        if (d.run() < 0) {
            return 1;
        }
    }

    sleep(5);  // Wait the inspectors to start up

    execvp(exec_argv[0], (char *const *) exec_argv);
    std::cerr << "Cannot execute " << exec_argv[0] << ": "
        << strerror(errno) << " (errno=" << errno << ")" << std::endl;
    return 1;
}

int cmd_wake(const char *prog, int argc, char **argv) noexcept {
    Discover d(TRAPIT_PORT, DiscoverMode::client);
    if (d.run() != 0) {
        return 1;
    }
    std::cout << d.pid() << std::endl;
    return 0;
}

Discover::Discover(in_port_t port, DiscoverMode mode) noexcept
    : m_fd(-1), m_port(port), m_mode(mode), m_pid(getpid()) {}

Discover::~Discover() {
    if (m_fd > 0) {
        close(m_fd);
    }
}

int Discover::set_timeout(uint32_t secs) noexcept {
    int r;
    struct timeval timeout = { .tv_sec = secs, .tv_usec = 0 };
    const void *opt = &timeout;
    socklen_t nopt = sizeof(timeout);

    r = setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, opt, nopt);
    if (r < 0) {
        return r;
    }

    r = setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, opt, nopt);
    if (r < 0) {
        return r;
    }

    return 0;
}

int Discover::run() noexcept {
    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0) {
        std::cerr << "Cannot create socket FD: " << strerror(errno)
            << " (errno=" << errno << ")" << std::endl;
        return m_fd;
    }

    struct sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port = htons(m_port),
        .sin_addr = {
            .s_addr = inet_addr("127.0.0.1"),
        },
    };

    switch (m_mode) {
        case DiscoverMode::server:
            if (bind(m_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
                std::cerr << "Cannot bind " << m_fd << "on 127.0.0.1:"
                    << m_port << ": " << strerror(errno) << " (errno="
                    << errno << ")" << std::endl;
                return -1;
            }
            while (1) {
                char cdata = '\0';
                ssize_t csize;
                struct sockaddr_in caddr;
                socklen_t ncaddr;
                csize = recvfrom(
                    m_fd, &cdata, sizeof(cdata), 0, (struct sockaddr *) &caddr,
                    &ncaddr);
                if (csize < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    std::cerr << "Cannot recvfrom " << m_fd << ":"
                        << strerror(errno) << " (errno=" << errno << ")"
                        << std::endl;
                    return csize;
                }
                if (ncaddr == 0) {
                    ncaddr = sizeof(caddr);  // Fix compatibility of macOS
                }

                pid_t pidh = pid();
                uint32_t pidn;
                char pidbuf[sizeof(pidn)];

                if (csize != sizeof(cdata) || cdata != '\n') {
                    pidh = 0;
                }
                pidn = htonl(pidh);
                memcpy(pidbuf, &pidn, sizeof(pidn));

                ssize_t nsent = sendto(
                    m_fd, pidbuf, sizeof(pidbuf), 0, (struct sockaddr *) &caddr,
                    ncaddr);
                if (nsent != sizeof(pidbuf)) {
                    continue;
                }

                if (pidh > 0) {
                    return 0;
                } else {
                    continue;
                }
            }
            break;
        case DiscoverMode::client:
            set_timeout(1);
            (void) connect(m_fd, (struct sockaddr *) &addr, sizeof(addr));

            while (1) {
                char cdata = '\n';
                ssize_t nsent = send(m_fd, &cdata, sizeof(cdata), 0);
                if (nsent == -1 && errno != EAGAIN) {
                    if (errno == EINTR) {
                        continue;
                    }
                    std::cerr << "Cannot contact the trapped process. "
                        << "Trying again. "
                        << strerror(errno) << " (errno=" << errno << ")"
                        << std::endl;
                    sleep(1);
                    continue;
                }

                uint32_t pidn;
                char pidbuf[sizeof(pidn)];
                while (1) {
                    ssize_t nrecv = recv(m_fd, &pidbuf, sizeof(pidbuf), 0);
                    if (nrecv == -1) {
                        if (errno == EINTR) {
                            continue;
                        }
                        if (errno == ECONNREFUSED) {
                            sleep(1);
                            break;
                        }
                        if (errno == EAGAIN) {
                            break;
                        }
                        std::cerr << "Cannot get PID of the trapped process: "
                            << strerror(errno) << " (errno=" << errno << ")"
                            << std::endl;
                        return 1;
                    }
                    memcpy(&pidn, pidbuf, sizeof(pidbuf));
                    m_pid = ntohl(pidn);
                    if (m_pid == 0) {
                        std::cerr << "Incorrect PID from the trapped process."
                            << std::endl;
                        return 1;
                    }
                    return 0;
                }
            }
            break;
    }

    return 0;
}

pid_t Discover::pid() const noexcept {
    return m_pid;
}
