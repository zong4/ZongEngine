namespace Hazel
{
	public static class Application
	{
		public static uint Width
		{
			get
			{
				unsafe { return InternalCalls.Application_GetWidth(); }
			}
		}

		public static uint Height
		{
			get
			{
				unsafe { return InternalCalls.Application_GetHeight(); }
			}
		}

		public static float AspectRatio => Width / (float)Height;

		public static string? DataDirectoryPath
		{
			get
			{
				unsafe { return InternalCalls.Application_GetDataDirectoryPath(); }
			}
		}

		public static string? GetSetting(string name, string defaultValue = "")
		{
			unsafe { return InternalCalls.Application_GetSetting(name, defaultValue); }
		}

		public static int GetSettingInt(string name, int defaultValue = 0)
		{
			unsafe { return InternalCalls.Application_GetSettingInt(name, defaultValue); }
		}

		public static float GetSettingFloat(string name, float defaultValue = 0.0f)
		{
			unsafe { return InternalCalls.Application_GetSettingFloat(name, defaultValue); }
		}

		public static void Quit()
 		{
 			unsafe { InternalCalls.Application_Quit(); }
 		}

	}
}
