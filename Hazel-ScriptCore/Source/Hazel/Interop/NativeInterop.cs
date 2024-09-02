using System;

namespace Hazel
{
    internal static class NativeInterop
	{
		/// <summary>
		/// Utility to promote e.g member fields to parameters that can be used as pointers.
		/// E.g replaces:
		/// <code>
		/// unsafe
		/// {
		///		fixed (ParamType* paramPtr = &param, paramPtr2 = &param2)
		///		{
		///			SomeFunc(paramPtr, paramPtr2);
		///		}
		/// }
		/// </code>
		/// with
		/// <code>
		/// NativeInterop.Fixed(param, (paramValue, paramValue2) =>
		/// {
		///		unsafe { SomeFunc(&paramValue, &paramValue2); }
		/// });
		/// </code>
		/// </summary>
		#region FixedUtility

		internal static TResult Fixed<TResult>(Func<TResult> func)
		{
			return func();
		}

		internal static TResult Fixed<T1, TResult>(T1 value,  Func<T1, TResult> func)
		{
			return func(value);
		}

		internal static TResult Fixed<T1, T2, TResult>(T1 value, T2 value2, Func<T1, T2, TResult> func)
		{
			return func(value, value2);
		}

		internal static TResult Fixed<T1, T2, T3, TResult>(T1 value, T2 value2, T3 value3, Func<T1, T2, T3, TResult> func)
		{
			return func(value, value2, value3);
		}

		internal static TResult Fixed<T1, T2, T3, T4, TResult>(T1 value, T2 value2, T3 value3, T4 value4, Func<T1, T2, T3, T4, TResult> func)
		{
			return func(value, value2, value3, value4);
		}

		internal static void Fixed<T1>(T1 value, Action<T1> func)
		{
			func(value);
		}

		internal static void Fixed<T1, T2>(T1 value, T2 value2, Action<T1, T2> func)
		{
			func(value, value2);
		}

		internal static void Fixed<T1, T2, T3>(T1 value, T2 value2, T3 value3, Action<T1, T2, T3> func)
		{
			func(value, value2, value3);
		}

		#endregion
	}
}
