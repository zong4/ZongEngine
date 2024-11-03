namespace Engine
{
	public static class Application
	{
		public static uint Width => InternalCalls.Application_GetWidth();
		public static uint Height => InternalCalls.Application_GetHeight();
		public static float AspectRatio => Width / (float)Height;

		public static string DataDirectoryPath => InternalCalls.Application_GetDataDirectoryPath();

		public static string GetSetting(string name, string defaultValue = "") => InternalCalls.Application_GetSetting(name, defaultValue);
		public static int GetSettingInt(string name, int defaultValue = 0) => InternalCalls.Application_GetSettingInt(name, defaultValue);
		public static float GetSettingFloat(string name, float defaultValue = 0.0f) => InternalCalls.Application_GetSettingFloat(name, defaultValue);


		public static void Quit() => InternalCalls.Application_Quit();

	}
}
