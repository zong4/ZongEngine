#pragma once

#include "Engine/Asset/Asset.h"

namespace Hazel {

	using ResourceDescriptorInfo = void*;

	class RendererResource : public Asset
	{
	public:
		virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;
	};

}
