#pragma once

#include "StreamWriter.h"
#include "StreamReader.h"
#include "Engine/Core/Buffer.h"

namespace Engine
{
	//==============================================================================
	/// MemoryStreamWriter
	class MemoryStreamWriter : public StreamWriter
	{
	public:
		MemoryStreamWriter(Buffer& buffer, size_t size);
		MemoryStreamWriter(const MemoryStreamWriter&) = delete;
		~MemoryStreamWriter();

		bool IsStreamGood() const final { return m_WritePos < m_Buffer.Size; }
		uint64_t GetStreamPosition() final { return m_WritePos; }
		void SetStreamPosition(uint64_t position) final { m_WritePos = position; }
		bool WriteData(const char* data, size_t size) final;

	private:
		Buffer& m_Buffer;
		size_t m_WritePos = 0;
	};

	//==============================================================================
	/// MemoryStreamReader
	class MemoryStreamReader : public StreamReader
	{
	public:
		MemoryStreamReader(const Buffer& buffer);
		MemoryStreamReader(const MemoryStreamReader&) = delete;
		~MemoryStreamReader();

		bool IsStreamGood() const final { return m_ReadPos < m_Buffer.Size; }
		uint64_t GetStreamPosition() final { return m_ReadPos; }
		void SetStreamPosition(uint64_t position) final { m_ReadPos = position; }
		bool ReadData(char* destination, size_t size) final;

	private:
		const Buffer& m_Buffer;
		size_t m_ReadPos = 0;
	};

} // namespace Engine
