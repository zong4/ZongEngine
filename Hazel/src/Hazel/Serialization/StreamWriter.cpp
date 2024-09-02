#include "hzpch.h"
#include "StreamWriter.h"

namespace Hazel
{
	void StreamWriter::WriteBuffer(Buffer buffer, bool writeSize)
	{
		if (writeSize)
			WriteData((char*)&buffer.Size, sizeof(uint64_t));

		WriteData((char*)buffer.Data, buffer.Size);
	}

	void StreamWriter::WriteZero(uint64_t size)
	{
		char zero = 0;
		for (uint64_t i = 0; i < size; i++)
			WriteData(&zero, 1);
	}

	void StreamWriter::WriteString(const std::string& string)
	{
		size_t size = string.size();
		WriteData((char*)&size, sizeof(size_t));
		WriteData((char*)string.data(), sizeof(char) * string.size());
	}

} // namespace Hazel
