using System;

namespace Engine
{
	public static class PerformanceTimers
	{
		public static float GetFrameTime() => InternalCalls.PerformanceTimers_GetFrameTime();
		public static float GetGPUTime() => InternalCalls.PerformanceTimers_GetGPUTime();
		public static float GetMainThreadWorkTime() => InternalCalls.PerformanceTimers_GetMainThreadWorkTime();
		public static float GetMainThreadWaitTime() => InternalCalls.PerformanceTimers_GetMainThreadWaitTime();
		public static float GetRenderThreadWorkTime() => InternalCalls.PerformanceTimers_GetRenderThreadWorkTime();
		public static float GetRenderThreadWaitTime() => InternalCalls.PerformanceTimers_GetRenderThreadWaitTime();
		public static uint GetFramesPerSecond() => InternalCalls.PerformanceTimers_GetFramesPerSecond();
		public static uint GetEntityCount() => InternalCalls.PerformanceTimers_GetEntityCount();
		public static uint GetScriptEntityCount() => InternalCalls.PerformanceTimers_GetScriptEntityCount();
	}
}
