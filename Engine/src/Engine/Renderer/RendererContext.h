#pragma once

#include "Engine/Core/Ref.h"

struct GLFWwindow;

namespace Engine {

	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Init() = 0;

		static Ref<RendererContext> Create();
	};

}
