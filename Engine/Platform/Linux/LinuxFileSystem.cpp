#include "pch.h"
#include "Engine/Utilities/FileSystem.h"
#include "Engine/Asset/AssetManager.h"

#include "Engine/Core/Application.h"

#include <GLFW/glfw3.h>

#include <sys/inotify.h>
#include <pthread.h>
#include <signal.h>

#include <filesystem>
#include <thread>

namespace Engine {

	static std::filesystem::path s_PersistentStoragePath;

	bool FileSystem::WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer)
	{
		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);

		if (!stream)
		{
			stream.close();
			return false;
		}

		stream.write((char*)buffer.Data, buffer.Size);
		stream.close();

		return true;
	}

	Buffer FileSystem::ReadBytes(const std::filesystem::path& filepath)
	{
		Buffer buffer;

		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);
		ZONG_CORE_ASSERT(stream);

		auto end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		auto size = end - stream.tellg();
		ZONG_CORE_ASSERT(size != 0);

		buffer.Allocate((uint32_t)size);
		stream.read((char*)buffer.Data, buffer.Size);
		stream.close();

		return buffer;
	}

	std::filesystem::path FileSystem::GetPersistentStoragePath()
	{
		if (!s_PersistentStoragePath.empty())
			return s_PersistentStoragePath;

		s_PersistentStoragePath = HasEnvironmentVariable("ENGINE_DIR") ? GetEnvironmentVariable("ENGINE_DIR") : "..";
		s_PersistentStoragePath /= "Editor";

		if (!std::filesystem::exists(s_PersistentStoragePath))
			std::filesystem::create_directory(s_PersistentStoragePath);

		return s_PersistentStoragePath;
	}

	bool FileSystem::HasEnvironmentVariable(const std::string& key)
	{
		return !GetEnvironmentVariable(key).empty();
	}

	bool FileSystem::SetEnvironmentVariable(const std::string& key, const std::string& value)
	{
		// NOTE(Emily): This looks horrible but the provided string *becomes* the environment for `putenv`
		// TODO(Emily): Persistent environment variables
		putenv((new std::string(fmt::format("{}={}", key, value)))->data());
		return true;
	}

	std::string FileSystem::GetEnvironmentVariable(const std::string& key)
	{
		const char* value = getenv(key.c_str());
		if (value)
			return std::string(value);
		else
			return {};
	}

}
