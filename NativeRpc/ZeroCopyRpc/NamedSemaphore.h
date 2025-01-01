#pragma once

#include <string>
#include <system_error>
#include <chrono>
#include "Export.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#endif

class EXPORT NamedSemaphore {
public:
    enum class OpenMode {
        Create,        // Create new, fail if exists
        Open,         // Open existing, fail if doesn't exist
        OpenOrCreate  // Open if exists, create if doesn't
    };

    NamedSemaphore() = delete;

    // Creates or opens a named semaphore with initial count
    NamedSemaphore(const std::string& name, OpenMode mode, unsigned int initialCount = 0);
    NamedSemaphore(const std::string& name, unsigned int initialCount = 0);

    ~NamedSemaphore();

    // Deleted copy constructor and assignment operator
    NamedSemaphore(const NamedSemaphore&) = delete;
    NamedSemaphore& operator=(const NamedSemaphore&) = delete;

    // Move constructor
    NamedSemaphore(NamedSemaphore&& other) noexcept;

    // Move assignment operator
    NamedSemaphore& operator=(NamedSemaphore&& other) noexcept;

    // Removes a named semaphore from the system
    static bool Remove(const std::string& name);

    // Acquires the semaphore (decrements count)
    void Acquire();

    // Tries to acquire the semaphore, returns true if successful
    bool TryAcquire();

    // Tries to acquire the semaphore with timeout
    template<typename Rep, typename Period>
    bool TryAcquireFor(const std::chrono::duration<Rep, Period>& timeout);

    // Releases the semaphore (increments count)
    void Release(unsigned int count = 1);

    // Gets current semaphore count (if supported by platform)
    int GetCount() const;

private:
#ifdef _WIN32
    HANDLE _handle = nullptr;
#else
    sem_t* _handle = nullptr;
#endif
    std::string _name;

    bool IsValid() const;

    static std::string FormatName(const std::string& name);
};

template <typename Rep, typename Period>
bool NamedSemaphore::TryAcquireFor(const std::chrono::duration<Rep, Period>& timeout)
{
#ifdef _WIN32
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
	DWORD result = WaitForSingleObject(_handle, static_cast<DWORD>(milliseconds.count()));
	if (result == WAIT_FAILED) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to try acquire semaphore with timeout");
	}
	return result == WAIT_OBJECT_0;
#else
        auto now = std::chrono::system_clock::now();
        auto systemTimeout = now + timeout;
        timespec ts;
        ts.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(systemTimeout.time_since_epoch()).count();
        ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
            systemTimeout.time_since_epoch() % std::chrono::seconds(1)).count();

        return sem_timedwait(_handle, &ts) == 0;
#endif
}