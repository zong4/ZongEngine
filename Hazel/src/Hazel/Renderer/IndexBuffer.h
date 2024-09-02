#pragma once

#include "Hazel/Core/Ref.h"

#include "RendererTypes.h"

namespace Hazel {

	class IndexBuffer : public RefCounted
	{
	public:
		virtual ~IndexBuffer() {}

		virtual void SetData(void* buffer, uint64_t size, uint64_t offset = 0) = 0;
		virtual void Bind() const = 0;

		virtual uint32_t GetCount() const = 0;

		virtual uint64_t GetSize() const = 0;
		virtual RendererID GetRendererID() const = 0;

		static Ref<IndexBuffer> Create(uint64_t size);
		static Ref<IndexBuffer> Create(void* data, uint64_t size = 0);
	};

}

