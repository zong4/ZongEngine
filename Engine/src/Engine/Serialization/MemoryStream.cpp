#include "hzpch.h"
#include "MemoryStream.h"

namespace Hazel
{
	//==============================================================================
	/// MemoryStreamWriter
	MemoryStreamWriter::MemoryStreamWriter(Buffer& buffer, size_t size)
		: m_Buffer(buffer)
	{
		if (size > buffer.Size)
			buffer.Allocate((uint32_t)size);
	}

	MemoryStreamWriter::~MemoryStreamWriter()
	{
	}

	bool MemoryStreamWriter::WriteData(const char* data, size_t size)
	{
		if (m_WritePos + size > m_Buffer.Size)
			return false;

		m_Buffer.Write(data, (uint32_t)size, (uint32_t)m_WritePos);
		return true;
	}

	//==============================================================================
	/// MemoryStreamReader
	MemoryStreamReader::MemoryStreamReader(const Buffer& buffer)
		: m_Buffer(buffer)
	{
	}

	MemoryStreamReader::~MemoryStreamReader()
	{
	}

	bool MemoryStreamReader::ReadData(char* destination, size_t size)
	{
		if (m_ReadPos + size > m_Buffer.Size)
			return false;

		memcpy(destination, (char*)m_Buffer.Data + m_ReadPos, size);
		return true;
	}

} // namespace Hazel
