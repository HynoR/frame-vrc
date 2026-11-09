// Adapted from frameclash-panel / frameeyeosc's panel/src/setup_tools.cpp (MIT, Copyright (c) 2026 konsti219;
// Copyright (c) 2026 sasaken1102r). See spawn.h.
#include "spawn.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

int64_t clockMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::vector<char*> cArgs(const std::vector<std::string>& argv) {
    std::vector<char*> args;
    for (const std::string& arg : argv) args.push_back(const_cast<char*>(arg.c_str()));
    args.push_back(nullptr);
    return args;
}

/** Nothing of the panel's (sockets, the GPU) goes along. */
void closeFrom3() {
#ifdef SYS_close_range
    if (::syscall(SYS_close_range, 3U, ~0U, 0U) == 0) return;
#endif
    for (int fd = 3; fd < 4096; ++fd) ::close(fd);
}

}  // namespace

bool inPath(const std::string& name) {
    if (name.find('/') != std::string::npos) return ::access(name.c_str(), X_OK) == 0;
    const char* path = std::getenv("PATH");
    const std::string dirs = path != nullptr ? path : "/usr/local/bin:/usr/bin:/bin";
    size_t from = 0;
    while (from <= dirs.size()) {
        const size_t colon = std::min(dirs.find(':', from), dirs.size());
        const std::string dir = dirs.substr(from, colon - from);
        if (!dir.empty() && ::access((dir + "/" + name).c_str(), X_OK) == 0) return true;
        from = colon + 1;
    }
    return false;
}

std::vector<std::string> detachedArgv(const std::vector<std::string>& argv, bool systemdRun) {
    if (!systemdRun) return argv;
    std::vector<std::string> out = {"systemd-run", "--user", "--collect", "--quiet"};
    for (const char* name : kSpawnEnvironment) {
        if (std::getenv(name) != nullptr) {
            out.push_back("-E");
            out.push_back(name);
        }
    }
    out.push_back("--");
    // systemd-run expands $VAR and turns "$$" into "$" in the command (it wrote "$" for a shell's "$$" pid):
    // "$$" is its escape for a literal "$"
    for (std::string arg : argv) {
        for (size_t i = arg.find('$'); i != std::string::npos; i = arg.find('$', i + 2)) arg.insert(i, 1, '$');
        out.push_back(arg);
    }
    return out;
}

bool spawnDetached(const std::vector<std::string>& program, std::string& error) {
    if (program.empty()) {
        error = "nothing to run";
        return false;
    }
    const std::vector<std::string> argv = detachedArgv(program, inPath("systemd-run"));
    std::vector<char*> args = cArgs(argv);
    const pid_t child = ::fork();
    if (child < 0) {
        error = std::string("fork: ") + std::strerror(errno);
        return false;
    }
    if (child == 0) {
        ::setsid();
        const pid_t grandchild = ::fork();
        if (grandchild != 0) ::_exit(grandchild < 0 ? 1 : 0);
        const int null = ::open("/dev/null", O_RDWR);
        if (null >= 0) {
            ::dup2(null, 0);
            ::dup2(null, 1);
            ::dup2(null, 2);
            if (null > 2) ::close(null);
        }
        closeFrom3();
        ::execvp(args[0], args.data());
        ::_exit(127);
    }
    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = "fork failed in the child";
        return false;
    }
    return true;
}

int capture(const std::vector<std::string>& argv, std::string& out, int timeoutMs) {
    out.clear();
    int fds[2];
    if (argv.empty() || ::pipe2(fds, O_CLOEXEC) != 0) return -1;
    std::vector<char*> args = cArgs(argv);
    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(fds[0]);
        ::close(fds[1]);
        return -1;
    }
    if (pid == 0) {
        ::setpgid(0, 0);
        const int null = ::open("/dev/null", O_RDWR);
        ::dup2(null, 0);
        ::dup2(fds[1], 1);
        ::dup2(null, 2);
        closeFrom3();
        ::execvp(args[0], args.data());
        ::_exit(127);
    }
    ::close(fds[1]);
    const int64_t until = clockMs() + timeoutMs;
    bool killed = false;
    for (;;) {
        pollfd p {fds[0], POLLIN, 0};
        const int left = static_cast<int>(std::max<int64_t>(0, until - clockMs()));
        const int r = ::poll(&p, 1, std::min(left, 200));
        if (r > 0) {
            char buf[4096];
            const ssize_t n = ::read(fds[0], buf, sizeof(buf));
            if (n > 0) {
                if (out.size() < (1u << 20)) out.append(buf, static_cast<size_t>(n));
                continue;
            }
            if (n == 0 || errno != EINTR) break;
        } else if (r < 0 && errno != EINTR) {
            break;
        }
        if (clockMs() >= until) {
            ::kill(-pid, SIGKILL);
            killed = true;
            break;
        }
    }
    ::close(fds[0]);
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (killed || !WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}
