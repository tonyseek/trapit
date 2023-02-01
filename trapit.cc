#include <iostream>
#include <cstring>

int cmd_usage(const char *prog);
int cmd_trap(const char *prog, int argc, char **argv);
int cmd_wake(const char *prog, int argc, char **argv);

int main(int argc, char **argv) {
    int _argc = argc - 2;
    char **_argv = argv + 2;

    if (_argc < 0) {
        return cmd_usage(argv[0]);
    } else if (std::strcmp(argv[1], "exec") == 0) {
        if (_argc > 0 && std::strcmp(_argv[0], "--") == 0) {
            _argc--;
            _argv++;
        }
        return cmd_trap(argv[0], _argc, _argv);
    } else if (std::strcmp(argv[1], "wake") == 0) {
        return cmd_wake(argv[0], _argc, _argv);
    } else if (std::strcmp(argv[1], "help") == 0) {
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
    std::cout << "Work-in-Progress: exec" << std::endl;
    while (argc > 0) {
        std::cout << "  " << *argv << std::endl;
        argc--;
        argv++;
    }
    return 0;
}

int cmd_wake(const char *prog, int argc, char **argv) {
    std::cout << "Work-in-Progress: wake" << std::endl;
    return 0;
}
