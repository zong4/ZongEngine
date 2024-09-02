using System;

namespace Hazel
{
	[AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    internal class EditorAssignableAttribute : Attribute {}
}
