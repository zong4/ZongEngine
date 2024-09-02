namespace Hazel
{
	public static class Log
	{
		internal enum LogLevel
		{
			Trace = 1 << 0,
			Debug = 1 << 1,
			Info = 1 << 2,
			Warn = 1 << 3,
			Error = 1 << 4,
			Critical = 1 << 5
		}

		public static void Trace(string format, params object[] parameters) => InternalCalls.Log_LogMessage(LogLevel.Trace, FormatUtils.Format(format, parameters));
		public static void Debug(string format, params object[] parameters) => InternalCalls.Log_LogMessage(LogLevel.Debug, FormatUtils.Format(format, parameters));
		public static void Info(string format, params object[] parameters) => InternalCalls.Log_LogMessage(LogLevel.Info, FormatUtils.Format(format, parameters));
		public static void Warn(string format, params object[] parameters) => InternalCalls.Log_LogMessage(LogLevel.Warn, FormatUtils.Format(format, parameters));
		public static void Error(string format, params object[] parameters) => InternalCalls.Log_LogMessage(LogLevel.Error, FormatUtils.Format(format, parameters));
		public static void Critical(string format, params object[] parameters) => InternalCalls.Log_LogMessage(LogLevel.Critical, FormatUtils.Format(format, parameters));

		public static void Trace(object value) => InternalCalls.Log_LogMessage(LogLevel.Trace, FormatUtils.Format(value));
		public static void Debug(object value) => InternalCalls.Log_LogMessage(LogLevel.Debug, FormatUtils.Format(value));
		public static void Info(object value) => InternalCalls.Log_LogMessage(LogLevel.Info, FormatUtils.Format(value));
		public static void Warn(object value) => InternalCalls.Log_LogMessage(LogLevel.Warn, FormatUtils.Format(value));
		public static void Error(object value) => InternalCalls.Log_LogMessage(LogLevel.Error, FormatUtils.Format(value));
		public static void Critical(object value) => InternalCalls.Log_LogMessage(LogLevel.Critical, FormatUtils.Format(value));
	}
}
