#pragma once

#include "InstrumentationTimer.hpp"
#include "Instrumentor.hpp"

#ifdef DEBUG
// Resolve which function signature macro will be used. Note that this only
// is resolved when the (pre)compiler starts, so the syntax highlighting
// could mark the wrong one in your editor!
    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
        #define ZONG_FUNC_SIG __PRETTY_FUNCTION__
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
        #define ZONG_FUNC_SIG __PRETTY_FUNCTION__
    #elif (defined(__FUNCSIG__) || (_MSC_VER))
        #define ZONG_FUNC_SIG __FUNCSIG__
    #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
        #define ZONG_FUNC_SIG __FUNCTION__
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
        #define ZONG_FUNC_SIG __FUNC__
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
        #define ZONG_FUNC_SIG __func__
    #elif defined(__cplusplus) && (__cplusplus >= 201103)
        #define ZONG_FUNC_SIG __func__
    #else
        #define ZONG_FUNC_SIG "ZONG_FUNC_SIG unknown!"
    #endif

    #define ZONG_PROFILE_BEGIN_SESSION(name, filepath) ::zong::core::Instrumentor::instance().beginSession(name, filepath)
    #define ZONG_PROFILE_END_SESSION() ::zong::core::Instrumentor::instance().endSession()
    #define ZONG_PROFILE_SCOPE_LINE2(name, line)                                                                  \
        constexpr auto                     fixedName##line = ::zong::core::CleanupOutputString(name, "__cdecl "); \
        ::zong::core::InstrumentationTimer timer##line(fixedName##line.Data)

    #define ZONG_PROFILE_SCOPE_LINE(name, line) ZONG_PROFILE_SCOPE_LINE2(name, line)
    #define ZONG_PROFILE_SCOPE(name) ZONG_PROFILE_SCOPE_LINE(name, __LINE__)
    #define ZONG_PROFILE_FUNCTION() ZONG_PROFILE_SCOPE(ZONG_FUNC_SIG)
#else
    #define ZONG_PROFILE_BEGIN_SESSION(name, filepath)
    #define ZONG_PROFILE_END_SESSION()
    #define ZONG_PROFILE_SCOPE(name)
    #define ZONG_PROFILE_FUNCTION()
#endif