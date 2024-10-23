using System;

namespace Hazel {

	public interface IEditorRunnable
	{
		/// <summary>
		/// Called once when this script is instantiated in the editor
		/// </summary>
		void OnInstantiate();

		/// <summary>
		/// Called once every frame that the UI is rendered
		/// </summary>
		void OnUIRender();

		/// <summary>
		/// Will be called when this script is being shut down, either because the component was removed, or because
		/// the editor is shutting down.
		/// </summary>
		void OnShutdown();
	}

}
