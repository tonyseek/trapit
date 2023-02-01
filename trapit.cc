// Copyright Jiangge Zhang (License: MIT)

#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <iostream>

#ifndef TRAPIT_MAX_ARGS
#define TRAPIT_MAX_ARGS 4096
#endif

int cmd_usage(const char *prog);
int cmd_trap(const char *prog, int argc, char **argv);
int cmd_wake(const char *prog, int argc, char **argv);

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

int cmd_usage(const char *prog) {
    const char *hl = "Usage: ";
    const char *pr = "       ";
    std::cerr << hl << prog << " [exec|wake|help]" << std::endl;
    std::cerr << pr << prog << " exec -- [argument ...]" << std::endl;
    std::cerr << pr << prog << " wake" << std::endl;
    return 2;
}

int cmd_trap(const char *prog, int argc, char **argv) {
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

    if (kill(getpid(), SIGSTOP) == -1) {
        std::cerr << "Cannot pause current process: " <<
            strerror(errno) << " (errno=" << errno << ")" << std::endl;
        return 1;
    }

    execvp(exec_argv[0], (char *const *) exec_argv);
    std::cerr << "Cannot execute " << exec_argv[0] << ": " <<
        strerror(errno) << " (errno=" << errno << ")" << std::endl;
    return 1;
}

int cmd_wake(const char *prog, int argc, char **argv) {
    std::cout << "Work-in-Progress: wake" << std::endl;
    return 0;
}
