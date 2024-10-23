namespace Hazel
{
	public static class SceneManager
	{
		/*
			public static ulong GetCurrentSceneID() => InternalCalls.SceneManager_GetCurrentSceneID();
		*/

		public static void LoadScene(Scene scene)
		{
			if (!scene.Handle.IsValid())
			{
				Log.Error("SceneManager: Tried to a scene with an invalid ID '{0}'", scene.Handle);
				return;
			}

			Scene.OnSceneChange();
			InternalCalls.SceneManager_LoadScene(ref scene.m_Handle);
		}

		public static string GetSceneName()
		{
			return InternalCalls.SceneManager_GetCurrentSceneName();
		}
	}
}
