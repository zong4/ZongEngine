#include "pch.h"
#include "NodeTypes.h"
#include "NodeDescriptors.h"

namespace Engine::SoundGraph
{
	/// Moving all constructors and Init() function calls for non-template nodes here
	/// to avoid recursive includes nightmare caused by trying to inline as much as possible.

	/// This macro should be used to define constructor and Init() function for node processor types
	/// that don't need to do anything in their constructor and Init() function
#define INIT_ENDPOINTS(TNodePocessor) TNodePocessor::TNodePocessor(const char* dbgName, UUID id) : NodeProcessor(dbgName, id) { EndpointUtilities::RegisterEndpoints(this); }\
							void TNodePocessor::Init() { EndpointUtilities::InitializeInputs(this); }

	/// This macro can be used if a node processor type needs to have some custom stuf in constructor
	/// or in its Init() function. In that case it has to declare 'void RegisterEndpoints()' and 'void InitializeInputs()'
	/// functions to be defined by this macro
#define INIT_ENDPOINTS_FUNCS(TNodePocessor) void TNodePocessor::RegisterEndpoints() { EndpointUtilities::RegisterEndpoints(this); }\
							void TNodePocessor::InitializeInputs() { EndpointUtilities::InitializeInputs(this); }

	INIT_ENDPOINTS(Modulo);
	INIT_ENDPOINTS(Power);
	INIT_ENDPOINTS(Log);
	INIT_ENDPOINTS(LinearToLogFrequency);
	INIT_ENDPOINTS(FrequencyLogToLinear);
	
	INIT_ENDPOINTS_FUNCS(Noise);
	INIT_ENDPOINTS_FUNCS(Sine);
	INIT_ENDPOINTS_FUNCS(WavePlayer);
	
	INIT_ENDPOINTS_FUNCS(ADEnvelope);

	INIT_ENDPOINTS_FUNCS(RepeatTrigger);
	INIT_ENDPOINTS_FUNCS(TriggerCounter);
	INIT_ENDPOINTS_FUNCS(DelayedTrigger);

	INIT_ENDPOINTS_FUNCS(BPMToSeconds);
	INIT_ENDPOINTS_FUNCS(FrequencyToNote);


} // namespace Engine::SoundGraph
