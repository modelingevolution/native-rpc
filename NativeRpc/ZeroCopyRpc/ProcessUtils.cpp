#include "ProcessUtils.h"

#ifdef WIN32

pid_t getCurrentProcessId() {
    return GetCurrentProcessId();
}


bool is_process_running(pid_t pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == nullptr) {
        return false; // Process does not exist
    }

    DWORD exitCode;
    bool running = GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE;
    CloseHandle(process);
    return running;
}

#else

bool is_process_running(pid_t pid) {
    // Send signal 0 to check if the process exists
    return (kill(pid, 0) == 0);
}

pid_t getCurrentProcessId() {
    return getpid();
}
#endif