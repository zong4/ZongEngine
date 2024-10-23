#include "pch.h"
#include "StreamReader.h"

namespace Hazel
{
	void StreamReader::ReadBuffer(Buffer& buffer, uint32_t size)
	{
		buffer.Size = size;
		if (size == 0)
			ReadData((char*)&buffer.Size, sizeof(uint32_t));

		buffer.Allocate(buffer.Size);
		ReadData((char*)buffer.Data, buffer.Size);
	}

	void StreamReader::ReadString(std::string& string)
	{
		size_t size;
		ReadData((char*)&size, sizeof(size_t));

		string.resize(size);
		ReadData((char*)string.data(), sizeof(char) * size);
	}

} // namespace Hazel
