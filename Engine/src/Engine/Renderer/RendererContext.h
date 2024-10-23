#pragma once

#include "Engine/Core/Ref.h"

struct GLFWwindow;

namespace Hazel {

	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Init() = 0;

		static Ref<RendererContext> Create();
	};

}
