#pragma once

#include "Hazel/Script/ScriptEngine.h"

#include "EditorPanel.h"

namespace Hazel {

	class ScriptEngineDebugPanel : public EditorPanel
	{
	public:
		ScriptEngineDebugPanel();
		~ScriptEngineDebugPanel();

		virtual void OnProjectChanged(const Ref<Project>& project) override;
		virtual void OnImGuiRender(bool& open) override;
	};

}
