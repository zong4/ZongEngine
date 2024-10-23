using System;

namespace Hazel
{
	internal static class FormatUtils
	{
		internal static string Format(string format, object[] parameters) => string.Format(format, parameters);
		internal static string Format(object value) => value != null ? value.ToString() : "null";
	}
}
