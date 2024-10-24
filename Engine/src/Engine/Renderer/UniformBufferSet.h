#pragma once

#include "UniformBuffer.h"

namespace Engine {

	class UniformBufferSet : public RefCounted
	{
	public:
		virtual ~UniformBufferSet() {}

		virtual Ref<UniformBuffer> Get() = 0;
		virtual Ref<UniformBuffer> RT_Get() = 0;
		virtual Ref<UniformBuffer> Get(uint32_t frame) = 0;

		virtual void Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame) = 0;

		static Ref<UniformBufferSet> Create(uint32_t size, uint32_t framesInFlight = 0);
	};

}
