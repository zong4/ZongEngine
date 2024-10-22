#include <Jolt/Jolt.h>
#include <Jolt/Core/Profiler.h>

namespace JPH {

#if defined(JPH_EXTERNAL_PROFILE)

	ExternalProfileMeasurement::ExternalProfileMeasurement(const char* inName, uint32 inColor /* = 0 */) {}
	ExternalProfileMeasurement::~ExternalProfileMeasurement() {}

#endif

}


