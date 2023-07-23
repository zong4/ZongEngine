#include "Instrumentor.hpp"

zong::core::Instrumentor& zong::core::Instrumentor::instance()
{
    static Instrumentor instance;
    return instance;
}

void zong::core::Instrumentor::beginSession(std::string const& name, std::string const& filepath)
{
    std::lock_guard lock(_mutex);

    if (_currentSession)
    {
        // if there is already a current session, then close it before beginning new one.
        // subsequent profiling output meant for the original session will end up in the
        // newly opened session instead.
        // that's better than having badly formatted profiling output.
        ZONG_CORE_ERROR("instrumentor::BeginSession('{0}') when session '{1}' already open.", name, _currentSession->_name);
        internalEndSession();
    }

    _outputStream.open(filepath);

    if (_outputStream.is_open())
    {
        _currentSession = new InstrumentationSession({name});
        writeHeader();
    }
    else
    {
        ZONG_CORE_ERROR("instrumentor could not open results file '{0}'.", filepath);
    }
}

void zong::core::Instrumentor::endSession()
{
    std::lock_guard lock(_mutex);
    internalEndSession();
}

void zong::core::Instrumentor::writeProfile(ProfileResult const& result)
{
    std::stringstream json;

    json << std::setprecision(3) << std::fixed;
    json << ",{";
    json << "\"cat\":\"function\",";
    json << "\"dur\":" << (result._elapsedTime.count()) << ',';
    json << "\"name\":\"" << result._name << "\",";
    json << "\"ph\":\"X\",";
    json << "\"pid\":0,";
    json << "\"tid\":" << result._threadID << ",";
    json << "\"ts\":" << result._start.count();
    json << "}";

    std::lock_guard lock(_mutex);

    if (_currentSession)
    {
        _outputStream << json.str();
        _outputStream.flush();
    }
}

void zong::core::Instrumentor::writeHeader()
{
    _outputStream << "{\"otherData\": {},\"traceEvents\":[{}";
    _outputStream.flush();
}

void zong::core::Instrumentor::writeFooter()
{
    _outputStream << "]}";
    _outputStream.flush();
}

void zong::core::Instrumentor::internalEndSession()
{
    if (_currentSession)
    {
        writeFooter();
        _outputStream.close();
        delete _currentSession;
        _currentSession = nullptr;
    }
}
