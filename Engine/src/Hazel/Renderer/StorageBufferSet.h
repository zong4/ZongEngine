#pragma once

#include "StorageBuffer.h"

namespace Hazel {

	class StorageBufferSet : public RefCounted
	{
	public:
		virtual ~StorageBufferSet() {}

		virtual Ref<StorageBuffer> Get() = 0;
		virtual Ref<StorageBuffer> RT_Get() = 0;
		virtual Ref<StorageBuffer> Get(uint32_t frame) = 0;

		virtual void Set(Ref<StorageBuffer> storageBuffer, uint32_t frame) = 0;
		virtual void Resize(uint32_t newSize) = 0;

		static Ref<StorageBufferSet> Create(const StorageBufferSpecification& specification, uint32_t size, uint32_t framesInFlight = 0);
	};

}
