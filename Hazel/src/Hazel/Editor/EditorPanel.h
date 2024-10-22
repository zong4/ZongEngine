#pragma once

#include <cstdlib>
#include "Hazel/Core/Ref.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Core/Events/Event.h"

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
