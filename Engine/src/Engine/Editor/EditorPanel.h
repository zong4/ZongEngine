#pragma once

#include <cstdlib>
#include "Engine/Core/Ref.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Project/Project.h"
#include "Engine/Core/Events/Event.h"

namespace Hazel {

	class EditorPanel : public RefCounted
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnImGuiRender(bool& isOpen) = 0;
		virtual void OnEvent(Event& e) {}
		virtual void OnProjectChanged(const Ref<Project>& project){}
		virtual void SetSceneContext(const Ref<Scene>& context){}
	};

}
