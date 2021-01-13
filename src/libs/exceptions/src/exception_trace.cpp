#include "exception_trace.h"
#include <exceptions/exceptions.h>
#include <csignal>
#include <cstring>// memset
#include <ctime>
#include <iostream>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>// sync
#include <vector>
#include <sstream>
#include <fstream>

#include <algorithm>
#include <set>
#if defined(__GNUC__) && __GNUC_PREREQ(7,5)
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#else
#include <filesystem>
namespace filesystem = std::filesystem;
#endif
#include "stacktrace.h"
#include <fmt/format.h>

namespace exceptions
{

namespace
{
std::set<std::string> ripfiles = {};
FILE* abortLogFileFd = nullptr;
int abortLogFileNrFd = 0;

#if defined(__APPLE__) and defined(__MACH__)
int gettid()
{
    uint64_t tid64 = 0;
    pthread_threadid_np(nullptr, &tid64);
    return static_cast<int>(tid64);
}

int tgkill(int /*tgid*/, int /*tid*/, int /*sig*/)
{
    return 0;
}
#else
int tgkill(int tgid, int tid, int sig)
{
    return syscall(__NR_tgkill, tgid, tid, sig);
}

int gettid()
{
    return syscall(__NR_gettid);
}
#endif

std::vector<pid_t> get_tids()
{
    pid_t pid = getpid();
    std::vector<pid_t> tids;
    const std::string procDir = fmt::format("/proc/{}/task", pid);

    if (!filesystem::exists(procDir))
    {
        return tids;
    }
    std::vector<filesystem::path> paths;
    std::copy(filesystem::directory_iterator(procDir),
              filesystem::directory_iterator(),
              std::back_inserter(paths));
    for (const auto& path : paths)
    {
        (void)path;
//        tids.push_back(static_cast<pid_t>(path.filename().native()));
    }
    std::sort(tids.begin(), tids.end());
    return tids;
}

std::string timestamp()
{
    const time_t now = std::time(nullptr);
    tm times{};
    gmtime_r(&now, &times);
    char timebuf[32] = {'\0'};
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &times);
    return timebuf;
}

}// namespace


namespace exception_trace
{

void register_rip_file(const std::string& name)
{
    ripfiles.insert(name);
}

void unregister_rip_file(const std::string& name)
{
    ripfiles.erase(name);
}

void message(const std::string& message_in)
{
    for (const auto& f : ripfiles)
    {
        std::ofstream file(f, std::ios::app);
        file << timestamp() << ':' << message_in << std::endl;
    }

    sync();
}

void exception(const std::exception& exception_in)
{
    message(exception_in.what());
}

namespace
{
void install_signal_handler(int sig, void (*handler)(int))
{
    struct sigaction sa
    {
    };
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (sigaction(sig, &sa, nullptr) != 0)
    {
        throw make_exception_with_call_stack_macro(std::runtime_error, "sigaction failed for sig " + std::to_string(sig) + "\n");
    }
}

void sf_silent_sig_handler(int /*sig*/)
{
    // do really really nothing please!
}

void sf_crash_handler(int sig)
{
    char name[20];
    pthread_getname_np(pthread_self(), name, sizeof(name));
    std::cerr << name << " ( SfCrashHandler ) Received signal" << sig << std::endl;
    std::ostringstream oss;
    oss << name << " SfCrashHandler for signal : " << sig << std::endl;
    oss << " CallStack : " << std::endl;
    oss << markusjx::stacktrace::stacktrace();
    exceptions::exception_trace::message(oss.str());
    std::cerr << " ( SfCrashHandler -> message) " << oss.str().c_str() << std::endl;
    // call default handler (to gather a core dump)
    signal(sig, SIG_DFL);
    raise(sig);
}

void sf_abort_handler(int sig)
{
    // avoid implementation that require malloc...
    std::fprintf(stderr, "EE Caught signal %d...\r\n", sig);
    if (abortLogFileFd)
    {
        std::fprintf(abortLogFileFd, "EE Caught signal %d...\r\n", sig);
        std::fprintf(abortLogFileFd, "EE Callstack:\r\n");
        //stacktrace_::safe_dump_to(abortLogFileNrFd);
        char name[20];
        pthread_getname_np(pthread_self(), name, sizeof(name));
        std::fprintf(abortLogFileFd, " Name: %19s\n", name);
        std::fprintf(abortLogFileFd, "Stack trace with symbols ....\n");
        if (!fflush(abortLogFileFd))// Only do the unsafe part once the flush has succeeded
        {
            std::ostringstream oss;
            oss << markusjx::stacktrace::stacktrace();
            std::fprintf(abortLogFileFd, "%s\n", oss.str().c_str());
            fflush(abortLogFileFd);
        }
    }
    // call default handler (to gather a core dump)
    signal(sig, SIG_DFL);
    raise(sig);
}

void sf_user1_sig_handler(int sig)
{
    std::cerr << "( SfUser1SigHandler ) Received signal" << sig << std::endl;
    std::ostringstream oss;
    oss << " SfUser1SigHandler for signal : " << sig << std::endl;
    oss << " TID : " << gettid() << std::endl;
    char name[20];
    pthread_getname_np(pthread_self(), name, sizeof(name));
    oss << " Name: " << name << std::endl;
    oss << " CallStack : " << std::endl;
    oss << markusjx::stacktrace::stacktrace();
    exceptions::exception_trace::message(oss.str());
    std::cerr << " ( SfUser1SigHandler -> message) " << oss.str().c_str() << std::endl;
}

void sf_user2_sig_handler(int sig)
{
    std::cerr << "( SfUser2SigHandler ) Received signal" << sig << std::endl;
    std::ostringstream oss;
    oss << " SfUser2SigHandler for signal : " << sig << std::endl;
    auto tids = get_tids();
    for (auto tid : tids)
    {
        tgkill(getpid(), tid, SIGUSR1);
    }
    exceptions::exception_trace::message(oss.str());
    std::cerr << " ( SfUser2SigHandler -> message) " << oss.str().c_str() << std::endl;
}


}// namespace

void install_crash_handlers(const std::string& storage)
{
    const std::string rip = storage + "/rip.txt";
    const std::string abort_log_file = storage + "/abort_log.txt";

    register_rip_file(rip);
    // move abort log into other rip files
    {
        std::ifstream input_file(abort_log_file);
        if (input_file)
        {
            std::string content((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
            if (content.length() > 0)
            {
                for (const auto& f : ripfiles)
                {
                    std::ofstream file(f, std::ios::app);
                    file << timestamp() << "(from abort logfile): " << content << std::endl;
                }
            }
            unlink(abort_log_file.c_str());
        }
    }

    // Open the abort_log file, because if there is a heap corruption, we can no longer open it
    abortLogFileFd = fopen(abort_log_file.c_str(), "wb");
    if (abortLogFileFd)
    {
        abortLogFileNrFd = fileno(abortLogFileFd);
    }

    // Ignore SIGPIPE
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        throw make_exception_with_call_stack_macro(std::runtime_error, "Failed to ignore SIGPIPE\n");
    }

    install_signal_handler(SIGIO, sf_silent_sig_handler);
    install_signal_handler(SIGSEGV, sf_crash_handler);
    install_signal_handler(SIGILL, sf_crash_handler);
    install_signal_handler(SIGABRT, sf_abort_handler);
    install_signal_handler(SIGUSR1, sf_user1_sig_handler);
    install_signal_handler(SIGUSR2, sf_user2_sig_handler);
}

}// namespace exception_trace

}// namespace exceptions
