using System;

namespace Hazel
{
	public static class PerformanceTimers
	{
		public static float GetFrameTime()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetFrameTime(); }
		}

		public static float GetGPUTime()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetGPUTime(); }
		}

		public static float GetMainThreadWorkTime()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetMainThreadWorkTime(); }
		}

		public static float GetMainThreadWaitTime()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetMainThreadWaitTime(); }
		}

		public static float GetRenderThreadWorkTime()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetRenderThreadWorkTime(); }
		}

		public static float GetRenderThreadWaitTime()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetRenderThreadWaitTime(); }
		}

		public static uint GetFramesPerSecond()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetFramesPerSecond(); }
		}

		public static uint GetEntityCount()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetEntityCount(); }
		}

		public static uint GetScriptEntityCount()
		{
			unsafe { return InternalCalls.PerformanceTimers_GetScriptEntityCount(); }
		}
	}
}
