#pragma once

#include "../log/Log.hpp"
#include "../time/Timer.hpp"
#include "core/pch.hpp"

namespace zong
{
namespace core
{

struct ProfileResult
{
    std::string     _name;
    TIME_UNIT       _start;
    TIME_UNIT       _elapsedTime;
    std::thread::id _threadID;
};

struct InstrumentationSession
{
    std::string _name;
};

/**
 * \brief create profile json file
 */
class Instrumentor
{
private:
    std::mutex              _mutex;
    InstrumentationSession* _currentSession;
    std::ofstream           _outputStream;

private:
    Instrumentor() : _currentSession(nullptr) {}
    ~Instrumentor() { endSession(); }

public:
    Instrumentor(Instrumentor const&)            = delete;
    Instrumentor(Instrumentor&&)                 = delete;
    Instrumentor& operator=(Instrumentor const&) = delete;

    static Instrumentor& instance();

    void beginSession(std::string const& name, std::string const& filepath = "results.json");
    void endSession();

    void writeProfile(ProfileResult const& result);

private:
    void writeHeader();

    void writeFooter();

    // note: you must already own lock on m_Mutex before calling InternalEndSession()
    void internalEndSession();
};

template <size_t N>
struct ChangeResult
{
    char Data[N];
};

template <size_t N, size_t K>
constexpr auto CleanupOutputString(const char (&expr)[N], const char (&remove)[K])
{
    ChangeResult<N> result = {};

    size_t srcIndex = 0;
    size_t dstIndex = 0;
    while (srcIndex < N)
    {
        size_t matchIndex = 0;
        while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
            matchIndex++;

        if (matchIndex == K - 1)
            srcIndex += matchIndex;

        result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
        srcIndex++;
    }

    return result;
}

} // namespace core
} // namespace zong
