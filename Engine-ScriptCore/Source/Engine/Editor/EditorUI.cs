using System;

namespace Engine
{
	public static class EditorUI
	{

		public static void Text(string format, params object[] args) => InternalCalls.EditorUI_Text(FormatUtils.Format(format, args));
		public static void Text(object value) => InternalCalls.EditorUI_Text(FormatUtils.Format(value));
		public static bool Button(string label, Vector2 size) => InternalCalls.EditorUI_Button(label, ref size);
		public static bool Button(string label) => Button(label, new Vector2(0.0f, 0.0f));

		public static bool BeginPropertyHeader(string label, bool openByDefault = true) => InternalCalls.EditorUI_BeginPropertyHeader(label, openByDefault);
		public static void EndPropertyHeader() => InternalCalls.EditorUI_EndPropertyHeader();

		public static void BeginPropertyGrid() => InternalCalls.EditorUI_PropertyGrid(true);
		public static void EndPropertyGrid() => InternalCalls.EditorUI_PropertyGrid(false);

		public static bool Property(string label, ref float value) => InternalCalls.EditorUI_PropertyFloat(label, ref value);
		public static bool Property(string label, ref Vector2 value) => InternalCalls.EditorUI_PropertyVec2(label, ref value);
		public static bool Property(string label, ref Vector3 value) => InternalCalls.EditorUI_PropertyVec3(label, ref value);
		public static bool Property(string label, ref Vector4 value) => InternalCalls.EditorUI_PropertyVec4(label, ref value);

	}
}
