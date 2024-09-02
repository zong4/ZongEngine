#include "hzpch.h"
#include "Hazel/Utilities/ProcessHelper.h"
#include "Hazel/Utilities/StringUtils.h"
#include "Hazel/Utilities/FileSystem.h"

#include <spawn.h>
#include <unistd.h>
#include <signal.h>

namespace Hazel {

	static std::unordered_map<UUID, pid_t> s_LinuxProcessStorage;

	UUID ProcessHelper::CreateProcess(const ProcessInfo& inProcessInfo)
	{
		std::filesystem::path workingDirectory = inProcessInfo.WorkingDirectory.empty() ? inProcessInfo.FilePath.parent_path() : inProcessInfo.WorkingDirectory;

		auto params = Utils::SplitString(inProcessInfo.CommandLine, ' ');

		std::string bin = inProcessInfo.FilePath.string();
		std::vector<char*> exec{ bin.data() };

		for(auto& s : params)
		{
			exec.emplace_back(s.data());
		}

		exec.push_back(nullptr);

		// TODO(Emily): Error handling
		pid_t pid;
		posix_spawnattr_t attr;
		int result = posix_spawnattr_init(&attr);
		if(result) HZ_CORE_VERIFY(false);

		std::string ld_lib_path = std::format("LD_LIBRARY_PATH={}", FileSystem::GetEnvironmentVariable("LD_LIBRARY_PATH"));
		char* env[] = {ld_lib_path.data(), nullptr};

		std::filesystem::path old = std::filesystem::current_path();
		std::filesystem::current_path(inProcessInfo.WorkingDirectory);
		if(posix_spawn(&pid, exec[0], nullptr, &attr, (char**) exec.data(), env)) HZ_CORE_VERIFY(false);
		std::filesystem::current_path(old);

		UUID processID = UUID();

		s_LinuxProcessStorage[processID] = pid;

		return processID;
	}

	void ProcessHelper::DestroyProcess(UUID inHandle, uint32_t inExitCode)
	{
		int result = kill(s_LinuxProcessStorage[inHandle], SIGTERM);
		if(result) HZ_CORE_VERIFY(false);
	}

}
