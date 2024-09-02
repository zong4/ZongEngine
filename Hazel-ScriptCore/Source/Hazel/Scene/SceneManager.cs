namespace Hazel
{
	public static class SceneManager
	{
		public static void LoadScene(Scene scene)
		{
			if (!scene.Handle.IsValid())
			{
				Log.Error("SceneManager: Tried to a scene with an invalid ID '{0}'", scene.Handle);
				return;
			}

			Scene.OnSceneChange();

			unsafe { InternalCalls.SceneManager_LoadScene(scene.Handle); }
		}

		public static string GetSceneName()
		{
			unsafe { return InternalCalls.SceneManager_GetCurrentSceneName(); }
		}
	}
}
